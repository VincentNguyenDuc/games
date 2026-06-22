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

static int random_variance(int base) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(-base / 4, base / 4);
    return base + dist(gen);
}

// Pure decision function — only reads registry, safe to run concurrently.
static AIDecision decide(Entity entity, BehaviorType type, int base_attack) {
    switch (type) {
    case BehaviorType::Wild:
        return {entity, random_variance(base_attack), 0};

    case BehaviorType::Schemer:
        // Splits damage between HP and essence to pressure the player's resources
        return {entity, random_variance(base_attack / 2), random_variance(base_attack / 2)};

    case BehaviorType::Guardian:
        // Heavy HP damage with 30% chance to double
        {
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
    auto* pos = reg.getComponent<Position>(player);
    Map* map = world.get_map(pos->map_id);

    // Collect living enemies in the map
    std::vector<std::pair<Entity, int>> enemies; // entity + base_attack
    for (Entity e : map->entities) {
        if (e == player)
            continue;
        auto* ai = reg.getComponent<AIBehavior>(e);
        auto* hp = reg.getComponent<Health>(e);
        auto* stats = reg.getComponent<Stats>(e);
        if (ai && hp && hp->hp > 0 && stats)
            enemies.emplace_back(e, stats->base_attack);
    }

    if (enemies.empty())
        return;

    // Concurrently decide — lambdas only read, no registry writes during this phase
    std::vector<std::future<AIDecision>> futures;
    for (auto [e, atk] : enemies) {
        auto type = reg.getComponent<AIBehavior>(e)->type;
        futures.push_back(std::async(std::launch::async, decide, e, type, atk));
    }

    // Apply sequentially after all decisions are collected
    auto* player_hp = reg.getComponent<Health>(player);
    auto* player_essence = reg.getComponent<PrimevalEssence>(player);

    for (auto& f : futures) {
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
