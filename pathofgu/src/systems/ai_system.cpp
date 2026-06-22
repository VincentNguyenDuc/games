#include "systems/ai_system.hpp"

#include "components/ai_behavior.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/stats.hpp"

#include <fmt/format.h>
#include <future>
#include <random>
#include <vector>

static int chebyshev(const Position& a, const Position& b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

// Move one cell toward target; prefers diagonal, falls back to axis-aligned.
// Refuses to enter Wall or Door cells.
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

static AIDecision decide(Entity entity, BehaviorType type, int base_attack) {
    switch (type) {
    case BehaviorType::Wild:
        return {entity, random_variance(base_attack), 0};

    case BehaviorType::Schemer:
        return {entity, random_variance(base_attack / 2), random_variance(base_attack / 2)};

    case BehaviorType::Guardian: {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        int dmg = random_variance(base_attack);
        if (dist(gen) < 0.30f)
            dmg *= 2;
        return {entity, dmg, 0};
    }
    }
    return {entity, 0, 0};
}

void ai_tick(EntityComponentRegistry& reg, World& world, Entity player) {
    auto* player_pos = reg.getComponent<Position>(player);
    auto* player_hp = reg.getComponent<Health>(player);
    auto* player_essence = reg.getComponent<PrimevalEssence>(player);

    Map* map = world.get_map(player_pos->map_id);

    // Sort enemies: out-of-range ones advance, in-range ones attack concurrently.
    std::vector<std::pair<Entity, std::future<AIDecision>>> attackers;

    for (Entity e : map->entities) {
        if (e == player)
            continue;

        auto* ai = reg.getComponent<AIBehavior>(e);
        auto* hp = reg.getComponent<Health>(e);
        auto* stats = reg.getComponent<Stats>(e);
        auto* epos = reg.getComponent<Position>(e);
        auto* name = reg.getComponent<Name>(e);

        if (!ai || !hp || hp->hp <= 0 || !stats || !epos)
            continue;

        int dist = chebyshev(*epos, *player_pos);

        if (dist > stats->attack_range) {
            step_toward(reg, world, e, *player_pos);
            fmt::print(
                "  {} advances toward you (distance {}).\n", name ? name->value : "Enemy", dist
            );
        } else {
            attackers.emplace_back(
                e, std::async(std::launch::async, decide, e, ai->type, stats->base_attack)
            );
        }
    }

    // Apply all attack decisions sequentially (no registry races)
    for (auto& [e, f] : attackers) {
        AIDecision d = f.get();
        auto* name = reg.getComponent<Name>(d.entity);

        if (d.hp_damage > 0) {
            player_hp->hp -= d.hp_damage;
            fmt::print("  {} strikes for {} damage!\n", name ? name->value : "Enemy", d.hp_damage);
        }
        if (d.essence_damage > 0) {
            player_essence->current = std::max(0, player_essence->current - d.essence_damage);
            fmt::print(
                "  {} disrupts your aperture, draining {} essence!\n",
                name ? name->value : "Enemy",
                d.essence_damage
            );
        }
    }
}
