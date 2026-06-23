#include "systems/ai_system.hpp"
#include "systems/combat_system.hpp"

#include "components/ai_behavior.hpp"
#include "components/aperture.hpp"
#include "components/attack_effect.hpp"
#include "components/health.hpp"
#include "components/move_intent.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/stats.hpp"
#include "items/gu_worm.hpp"
#include "utility.hpp"

#include <fmt/format.h>
#include <future>
#include <random>
#include <vector>

// Stamps a movement intent toward `to`; resolve_moves() will apply it with slide fallback.
static void move_toward(
    EntityComponentRegistry& reg, Entity e, const Position& from, const Position& to
) {
    int dx = (to.x > from.x) - (to.x < from.x);
    int dy = (to.y > from.y) - (to.y < from.y);
    reg.addComponent(e, MoveIntentComponent{dx, dy, /*allow_slide=*/true});
}

// Snapshot of an enemy, safe to copy for async use.
// GuWorm::def is an immutable shared_ptr flyweight.
struct EnemySnapshot {
    Entity entity;
    BehaviorType behavior;
    int base_attack;
    int hp, max_hp;
    int essence;
    std::vector<GuWorm> worms;
};

static AIDecision decide(EnemySnapshot ctx) {
    float hp_ratio = (float)ctx.hp / (float)std::max(1, ctx.max_hp);
    float heal_threshold = (ctx.behavior == BehaviorType::Wild) ? 0.30f : 0.50f;

    // Try recovery worm when critically low on HP
    if (hp_ratio < heal_threshold) {
        for (int i = 0; i < (int)ctx.worms.size(); ++i) {
            const auto& def = *ctx.worms[i].def;
            if (def.type == GuWormType::Recovery && ctx.essence >= def.essence_cost)
                return {ctx.entity, 0, 0, i, true};
        }
    }

    // Schemers prefer defensive worms when moderately hurt
    if (ctx.behavior == BehaviorType::Schemer && hp_ratio < 0.70f) {
        for (int i = 0; i < (int)ctx.worms.size(); ++i) {
            const auto& def = *ctx.worms[i].def;
            if (def.type == GuWormType::Defensive && ctx.essence >= def.essence_cost)
                return {ctx.entity, 0, 0, i, false};
        }
    }

    // Pick first usable offensive or defensive worm
    for (int i = 0; i < (int)ctx.worms.size(); ++i) {
        const auto& def = *ctx.worms[i].def;
        if ((def.type == GuWormType::Offensive || def.type == GuWormType::Defensive) &&
            ctx.essence >= def.essence_cost)
            return {ctx.entity, 0, 0, i, false};
    }

    // No usable worm — raw stat attack
    int dmg = random_variance(ctx.base_attack);
    if (ctx.behavior == BehaviorType::Guardian) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> p(0.0f, 1.0f);
        if (p(gen) < 0.30f)
            dmg *= 2; // power strike
    }
    int ess_drain =
        (ctx.behavior == BehaviorType::Schemer) ? random_variance(ctx.base_attack / 2) : 0;
    return {ctx.entity, dmg, ess_drain, -1, false};
}

std::string ai_tick(EntityComponentRegistry& reg, World& world, Entity player) {
    auto* player_pos = reg.getComponent<Position>(player);

    Map* map = world.get_map(player_pos->map_id);

    std::string messages;
    std::vector<std::pair<Entity, std::future<AIDecision>>> pending;

    for (Entity e : map->entities) {
        if (e == player)
            continue;

        auto* ai = reg.getComponent<AIBehavior>(e);
        auto* hp = reg.getComponent<Health>(e);
        auto* stats = reg.getComponent<Stats>(e);
        auto* epos = reg.getComponent<Position>(e);
        auto* name = reg.getComponent<Name>(e);
        auto* ap = reg.getComponent<Aperture>(e);
        auto* ess = reg.getComponent<PrimevalEssence>(e);

        if (!ai || !hp || hp->hp <= 0 || !stats || !epos)
            continue;

        // Small regen so enemies don't run permanently dry
        if (ess && ess->current < ess->max)
            ess->current = std::min(ess->max, ess->current + 5);

        int dist = chebyshev(*epos, *player_pos);

        if (dist > stats->attack_range) {
            move_toward(reg, e, *epos, *player_pos);
            messages += fmt::format("{} advances toward you.\n", name ? name->value : "Enemy");
        } else {
            EnemySnapshot snap{
                e,
                ai->type,
                stats->base_attack,
                hp->hp,
                hp->max_hp,
                ess ? ess->current : 0,
                ap ? ap->worms : std::vector<GuWorm>{},
            };
            pending.emplace_back(e, std::async(std::launch::async, decide, snap));
        }
    }

    // Apply decisions sequentially (no registry races).
    // Stamp effect components — resolve_*_effects() applies them afterward.
    for (auto& [e, f] : pending) {
        AIDecision d = f.get();

        if (d.worm_slot >= 0) {
            // Worm activation: heal self or attack player
            Entity effect_target = d.heal_self ? d.entity : player;
            activate_worm(reg, d.entity, effect_target, d.worm_slot);
        } else {
            // Raw stat attack → stamp AttackEffectComponent directly
            if (d.hp_damage > 0)
                reg.addComponent(
                    d.entity, AttackEffectComponent{player, d.hp_damage, {}, GuWormType::Offensive}
                );
            // Schemer essence drain → Support type attack effect
            if (d.essence_damage > 0)
                reg.addComponent(
                    d.entity,
                    AttackEffectComponent{player, d.essence_damage, {}, GuWormType::Support}
                );
        }
    }

    return messages; // movement messages only; combat messages come from effect resolution
}
