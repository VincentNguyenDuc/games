#include "systems/ai_system.hpp"
#include "systems/combat_system.hpp"

#include "components/ai_behavior.hpp"
#include "components/aperture.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/stats.hpp"
#include "items/gu_worm.hpp"

#include <fmt/format.h>
#include <future>
#include <random>
#include <vector>

static int chebyshev(const Position& a, const Position& b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

static void step_toward(
    EntityComponentRegistry& reg, World& world, Entity enemy, const Position& target
) {
    auto* pos = reg.getComponent<Position>(enemy);
    if (!pos)
        return;

    Map* map = world.get_map(pos->map_id);
    int dx = (target.x > pos->x) - (target.x < pos->x);
    int dy = (target.y > pos->y) - (target.y < pos->y);

    auto passable = [&](int x, int y) {
        return x >= 0 && x < 10 && y >= 0 && y < 10 && map->grid[y][x] == Cell::Empty;
    };

    if (dx && dy && passable(pos->x + dx, pos->y + dy)) {
        pos->x += dx;
        pos->y += dy;
    } else if (dx && passable(pos->x + dx, pos->y)) {
        pos->x += dx;
    } else if (dy && passable(pos->x, pos->y + dy)) {
        pos->y += dy;
    }
}

static int random_variance(int base) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(-base / 4, base / 4);
    return base + dist(gen);
}

// Immutable snapshot of an enemy, safe to capture by value for async use.
// GuWorm::def is a shared_ptr flyweight — copying is cheap and the def is immutable.
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

    // Heal threshold: Wild heals only when nearly dead; others are more cautious
    float heal_threshold = (ctx.behavior == BehaviorType::Wild) ? 0.30f : 0.50f;

    // Try a recovery worm when HP is critically low
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

    // Pick first usable offensive (or defensive) worm
    for (int i = 0; i < (int)ctx.worms.size(); ++i) {
        const auto& def = *ctx.worms[i].def;
        if ((def.type == GuWormType::Offensive || def.type == GuWormType::Defensive) &&
            ctx.essence >= def.essence_cost)
            return {ctx.entity, 0, 0, i, false};
    }

    // No usable worm — fall back to raw stat-based attack
    int dmg = random_variance(ctx.base_attack);
    if (ctx.behavior == BehaviorType::Guardian) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> p(0.0f, 1.0f);
        if (p(gen) < 0.30f)
            dmg *= 2; // Guardian power strike
    }
    int ess_drain =
        (ctx.behavior == BehaviorType::Schemer) ? random_variance(ctx.base_attack / 2) : 0;
    return {ctx.entity, dmg, ess_drain, -1, false};
}

std::string ai_tick(EntityComponentRegistry& reg, World& world, Entity player) {
    auto* player_pos = reg.getComponent<Position>(player);
    auto* player_hp = reg.getComponent<Health>(player);
    auto* player_essence = reg.getComponent<PrimevalEssence>(player);

    Map* map = world.get_map(player_pos->map_id);

    std::string messages;
    std::vector<std::pair<Entity, std::future<AIDecision>>> attackers;

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

        // Small essence regen per turn so enemies don't run dry permanently
        if (ess && ess->current < ess->max)
            ess->current = std::min(ess->max, ess->current + 5);

        int dist = chebyshev(*epos, *player_pos);

        if (dist > stats->attack_range) {
            step_toward(reg, world, e, *player_pos);
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
            attackers.emplace_back(e, std::async(std::launch::async, decide, snap));
        }
    }

    // Apply all decisions sequentially (no registry races)
    for (auto& [e, f] : attackers) {
        AIDecision d = f.get();
        auto* name = reg.getComponent<Name>(d.entity);
        std::string ename = name ? name->value : "Enemy";

        if (d.worm_slot >= 0) {
            // enemy_entity plays "player" role, human player plays "target" role:
            //   Recovery worm → heals d.entity (first arg)
            //   Offensive worm → damages player (second arg)
            auto result = player_attack(reg, d.entity, player, d.worm_slot);
            if (!result.success) {
                // worm use failed (out of essence mid-turn etc.) — do nothing
            } else if (d.heal_self) {
                auto* ehp = reg.getComponent<Health>(d.entity);
                messages += fmt::format(
                    "{} uses a recovery worm! HP restored to {}/{}.\n",
                    ename,
                    ehp ? ehp->hp : 0,
                    ehp ? ehp->max_hp : 0
                );
            } else {
                auto* ap = reg.getComponent<Aperture>(d.entity);
                std::string wname = (ap && d.worm_slot < (int)ap->worms.size())
                                        ? ap->worms[d.worm_slot].def->name
                                        : "a worm";
                messages += fmt::format(
                    "{} activates {}! Deals {} damage to you.\n", ename, wname, result.damage_dealt
                );
            }
        } else {
            if (d.hp_damage > 0) {
                player_hp->hp -= d.hp_damage;
                messages += fmt::format("{} strikes for {} damage!\n", ename, d.hp_damage);
            }
            if (d.essence_damage > 0) {
                player_essence->current = std::max(0, player_essence->current - d.essence_damage);
                messages += fmt::format(
                    "{} disrupts your aperture, draining {} essence!\n", ename, d.essence_damage
                );
            }
        }
    }

    return messages;
}
