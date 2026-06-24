#include "systems/ai_system.hpp"
#include "actions/combat.hpp"

#include "components/ai_behavior.hpp"
#include "components/aperture.hpp"
#include "components/attack_effect.hpp"
#include "components/health.hpp"
#include "components/move_intent.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/self_effect.hpp"
#include "components/stats.hpp"
#include "items/gu_worm.hpp"
#include "utility.hpp"

#include <fmt/format.h>
#include <future>
#include <random>
#include <vector>

struct AIDecision {
    Entity entity;
    int hp_damage = 0;
    int essence_damage = 0;
    int worm_slot = -1; // -1 = raw stat attack; >= 0 = aperture slot
    bool heal_self = false;
};

static void move_toward(
    EntityComponentRegistry& reg, Entity e, const Position& from, const Position& to
) {
    int dx = (to.x > from.x) - (to.x < from.x);
    int dy = (to.y > from.y) - (to.y < from.y);
    reg.addComponent(e, MoveIntentComponent{dx, dy, /*allow_slide=*/true});
}

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

    if (hp_ratio < heal_threshold) {
        for (int i = 0; i < (int)ctx.worms.size(); ++i) {
            const auto& def = *ctx.worms[i].def;
            if (def.type == GuWormType::Recovery && ctx.essence >= def.essence_cost)
                return {ctx.entity, 0, 0, i, true};
        }
    }

    if (ctx.behavior == BehaviorType::Schemer && hp_ratio < 0.70f) {
        for (int i = 0; i < (int)ctx.worms.size(); ++i) {
            const auto& def = *ctx.worms[i].def;
            if (def.type == GuWormType::Defensive && ctx.essence >= def.essence_cost)
                return {ctx.entity, 0, 0, i, false};
        }
    }

    for (int i = 0; i < (int)ctx.worms.size(); ++i) {
        const auto& def = *ctx.worms[i].def;
        if ((def.type == GuWormType::Offensive || def.type == GuWormType::Defensive) &&
            ctx.essence >= def.essence_cost)
            return {ctx.entity, 0, 0, i, false};
    }

    int dmg = random_variance(ctx.base_attack);
    if (ctx.behavior == BehaviorType::Guardian) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> p(0.0f, 1.0f);
        if (p(gen) < 0.30f)
            dmg *= 2;
    }
    int ess_drain =
        (ctx.behavior == BehaviorType::Schemer) ? random_variance(ctx.base_attack / 2) : 0;
    return {ctx.entity, dmg, ess_drain, -1, false};
}

AiTickSystem::AiTickSystem(GameWorld& gw, Entity p)
    : game_world(gw)
    , player(p) {
    reads = std::vector<ComponentType>{
        ComponentType(typeid(Position)),
        ComponentType(typeid(AIBehavior)),
        ComponentType(typeid(Health)),
        ComponentType(typeid(Stats)),
        ComponentType(typeid(Aperture)),
        ComponentType(typeid(PrimevalEssence)),
        ComponentType(typeid(Name)),
    };
    writes = std::vector<ComponentType>{
        ComponentType(typeid(MoveIntentComponent)),
        ComponentType(typeid(SelfEffectComponent)),
        ComponentType(typeid(AttackEffectComponent)),
        ComponentType(typeid(PrimevalEssence)),
    };
}

void AiTickSystem::update(EntityComponentRegistry& reg, CommandBuffer&) {
    auto* player_pos = reg.getComponent<Position>(player);
    Map* map = game_world.get_map(player_pos->map_id);

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

    for (auto& [e, f] : pending) {
        AIDecision d = f.get();

        if (d.worm_slot >= 0) {
            Entity effect_target = d.heal_self ? d.entity : player;
            activate_worm(reg, d.entity, effect_target, d.worm_slot);
        } else {
            if (d.hp_damage > 0)
                reg.addComponent(
                    d.entity, AttackEffectComponent{player, d.hp_damage, {}, GuWormType::Offensive}
                );
            if (d.essence_damage > 0)
                reg.addComponent(
                    d.entity,
                    AttackEffectComponent{player, d.essence_damage, {}, GuWormType::Support}
                );
        }
    }

    output = messages;
}
