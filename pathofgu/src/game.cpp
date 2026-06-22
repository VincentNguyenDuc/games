#include "game.hpp"

#include "components/ai_behavior.hpp"
#include "components/aperture.hpp"
#include "components/cultivation_rank.hpp"
#include "components/health.hpp"
#include "components/loot.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/stats.hpp"
#include "input/input_thread.hpp"
#include "systems/ai_system.hpp"
#include "systems/combat_system.hpp"
#include "systems/loot_system.hpp"
#include "systems/movement_system.hpp"
#include "systems/render_system.hpp"

#include <algorithm>
#include <fmt/format.h>

Game::Game()
    : reg_(std::make_shared<EntityComponentRegistry>())
    , world_(std::make_unique<World>())
    , db_(std::make_unique<InMemoryGuWormDatabase>()) {}

void Game::run() {
    spawn_player();
    spawn_enemies();

    auto input_thread = launch_input_thread(queue_);

    fmt::print("\n");
    fmt::print("  PATH OF GU\n");
    fmt::print("  You awaken inside the collapsing Grotto-Heaven of a deceased Rank 6 Immortal.\n");
    fmt::print("  Fight through to the Guardian's Sanctum and escape.\n");

    render(*reg_, *world_, player_);

    while (running_) {
        auto cmd = queue_.pop_blocking();
        process_command(cmd);
    }
}

// ── Spawning ────────────────────────────────────────────────────────────────

void Game::spawn_player() {
    player_ = entity_manager_.createEntity();
    auto strength_gu = db_->get("Strength Gu");

    reg_->addComponent(player_, Name{"Fang Yuan"});
    reg_->addComponent(player_, Health{100, 100});
    reg_->addComponent(player_, PrimevalEssence{60, 60, 0});
    reg_->addComponent(player_, CultivationRank{1, 0});
    reg_->addComponent(player_, Aperture{{{strength_gu}}, 3});
    reg_->addComponent(player_, Position{world_->entrance_id(), 5, 5});

    world_->add_entity(world_->entrance_id(), player_);
}

Entity Game::make_enemy(
    MapId map,
    const std::string& name,
    int hp,
    int base_attack,
    int base_defense,
    BehaviorType behavior,
    std::vector<GuWormDrop> drops
) {
    Entity e = entity_manager_.createEntity();
    reg_->addComponent(e, Name{name});
    reg_->addComponent(e, Health{hp, hp});
    int range = (behavior == BehaviorType::Guardian)  ? 2
                : (behavior == BehaviorType::Schemer) ? 3
                                                      : 1;
    reg_->addComponent(e, Stats{base_attack, base_defense, range});
    reg_->addComponent(e, AIBehavior{behavior});
    reg_->addComponent(e, Loot{std::move(drops)});
    auto [ex, ey] = world_->random_empty_cell(map);
    reg_->addComponent(e, Position{map, ex, ey});
    world_->add_entity(map, e);
    return e;
}

void Game::spawn_enemies() {
    // Map IDs match World constructor order: 0=entrance, 1=passage, 2=hive,
    // 3=hall, 4=refinery, 5=cache(loot only), 6=sealed, 7=sanctum(boss)
    auto strength_gu = db_->get("Strength Gu");
    auto iron_skin_gu = db_->get("Iron Skin Gu");
    auto lightning_gu = db_->get("Lightning Gu");
    auto jade_skin_gu = db_->get("Jade Skin Gu");
    auto vital_gu = db_->get("Vital Gu");
    auto thunder_gu = db_->get("Thunder Stomp Gu");

    // Map 1 — Spirit Stone Passage
    make_enemy(1, "Wild Strength Gu", 18, 6, 0, BehaviorType::Wild, {{{strength_gu}, 0.80f}});

    // Map 2 — Worm Hive Chamber
    make_enemy(2, "Wild Strength Gu", 18, 6, 0, BehaviorType::Wild, {{{strength_gu}, 0.60f}});
    make_enemy(2, "Wild Iron Skin Worm", 22, 4, 3, BehaviorType::Wild, {{{iron_skin_gu}, 0.60f}});

    // Map 3 — Bloodstained Hall (first Schemer)
    make_enemy(
        3,
        "Demonic Gu Master - Wei",
        45,
        14,
        2,
        BehaviorType::Schemer,
        {{{lightning_gu}, 0.70f}, {{iron_skin_gu}, 0.50f}}
    );

    // Map 4 — Ancient Refinement Chamber
    make_enemy(
        4,
        "Demonic Gu Master - Liu",
        50,
        16,
        3,
        BehaviorType::Schemer,
        {{{jade_skin_gu}, 0.70f}, {{vital_gu}, 0.40f}}
    );
    make_enemy(
        4, "Corrupted Worm Construct", 28, 8, 2, BehaviorType::Wild, {{{strength_gu}, 0.50f}}
    );

    // Map 5 — Forgotten Cache: pre-place loot, no enemies
    Map* cache = world_->get_map(5);
    cache->dropped_worms.push_back({db_->get("Moonlight Gu")});
    cache->dropped_worms.push_back({db_->get("Steel Bones Gu")});

    // Map 6 — Sealed Side Chamber (mini-boss construct)
    make_enemy(
        6,
        "Iron Guardian Construct",
        65,
        18,
        5,
        BehaviorType::Guardian,
        {{{db_->get("Rock Skin Gu")}, 1.0f}, {{thunder_gu}, 0.50f}}
    );

    // Map 7 — Guardian's Sanctum (final boss)
    make_enemy(
        7,
        "Immortal's Guardian",
        120,
        28,
        8,
        BehaviorType::Guardian,
        {{{db_->get("Boiling Blood Gu")}, 1.0f}, {{db_->get("Fixed Immortal Gu")}, 0.60f}}
    );
}

// ── Command handling ─────────────────────────────────────────────────────────

void Game::process_command(const PlayerCommand& cmd) {
    std::visit(
        [this](const auto& c) {
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, QuitCommand>) {
            fmt::print("You abandon your path. The Grotto-Heaven swallows you whole.\n");
            running_ = false;
            return;
        }

        if constexpr (std::is_same_v<T, MoveCommand>) {
            if (move_player(*reg_, *world_, player_, c.direction)) {
                auto* essence = reg_->getComponent<PrimevalEssence>(player_);
                int regen = std::max(1, essence->max / 5);
                essence->current = std::min(essence->max, essence->current + regen);
                essence->depleted_turns = 0;
            }
        }

        if constexpr (std::is_same_v<T, AttackCommand>) {
            auto* pos = reg_->getComponent<Position>(player_);
            Map* map = world_->get_map(pos->map_id);

            // Collect living enemies
            std::vector<Entity> enemies;
            for (Entity e : map->entities) {
                if (e != player_ && reg_->getComponent<Health>(e))
                    enemies.push_back(e);
            }

            if (enemies.empty()) {
                fmt::print("No enemies here.\n");
                return;
            }

            // Default target: first enemy
            Entity target = enemies[0];
            auto result = player_attack(*reg_, player_, target, c.worm_slot);
            fmt::print("{}\n", result.message);

            if (result.success) {
                if (result.target_died) {
                    fmt::print("Loot:\n");
                    process_death(*reg_, *world_, target);
                    destroy_entity(target);
                }
                post_action_tick();
            }
        }

        if constexpr (std::is_same_v<T, PickupCommand>) {
            pickup_worm(*reg_, *world_, player_, c.pickup_index);
        }

        if constexpr (std::is_same_v<T, DropCommand>) {
            drop_worm(*reg_, *world_, player_, c.pickup_index);
        }

        if constexpr (std::is_same_v<T, SkipCommand>) {
            fmt::print("You focus and let your essence settle.\n");
            auto* essence = reg_->getComponent<PrimevalEssence>(player_);
            if (!is_in_combat()) {
                int regen = std::max(1, essence->max / 5);
                essence->current = std::min(essence->max, essence->current + regen);
            }
            post_action_tick();
        }
        },
        cmd);

    if (!running_)
        return;

    if (check_win())
        return;

    render(*reg_, *world_, player_);
}

void Game::post_action_tick() {
    auto* essence = reg_->getComponent<PrimevalEssence>(player_);

    if (is_in_combat()) {
        fmt::print("\nEnemies act:\n");
        ai_tick(*reg_, *world_, player_);
        check_deaths();

        if (essence->current == 0) {
            essence->depleted_turns++;
            if (essence->depleted_turns >= 3) {
                fmt::print("\nYour aperture has collapsed — essence depleted for 3 turns.\n");
                fmt::print("DEFEAT.\n");
                running_ = false;
                return;
            }
            fmt::print(
                "WARNING: Aperture at risk — essence depleted ({}/3 turns)!\n",
                essence->depleted_turns
            );
        } else {
            essence->depleted_turns = 0;
        }
    }
}

void Game::check_deaths() {
    auto* player_hp = reg_->getComponent<Health>(player_);
    if (player_hp->hp <= 0) {
        fmt::print("\nYou have fallen. Your Gu worms scatter into the void.\n");
        fmt::print("DEFEAT.\n");
        running_ = false;
    }
}

bool Game::check_win() {
    auto* pos = reg_->getComponent<Position>(player_);
    Map* map = world_->get_map(pos->map_id);
    if (!map->is_exit)
        return false;

    // Must clear the map first
    for (Entity e : map->entities) {
        if (e != player_ && reg_->getComponent<Health>(e))
            return false;
    }

    fmt::print("\n==========================================\n");
    fmt::print("  The exit seal shatters. You step through.\n");
    fmt::print("  The Grotto-Heaven collapses behind you.\n");
    fmt::print("  You have escaped. Your path continues.\n");
    fmt::print("  VICTORY.\n");
    fmt::print("==========================================\n");
    running_ = false;
    return true;
}

bool Game::is_in_combat() const {
    auto* pos = reg_->getComponent<Position>(player_);
    Map* map = world_->get_map(pos->map_id);
    for (Entity e : map->entities) {
        if (e != player_ && reg_->getComponent<AIBehavior>(e) && reg_->getComponent<Health>(e))
            return true;
    }
    return false;
}

void Game::destroy_entity(Entity e) {
    auto* pos = reg_->getComponent<Position>(e);
    if (pos)
        world_->remove_entity(pos->map_id, e);

    reg_->removeComponent<Health>(e);
    reg_->removeComponent<Stats>(e);
    reg_->removeComponent<AIBehavior>(e);
    reg_->removeComponent<Loot>(e);
    reg_->removeComponent<Aperture>(e);
    reg_->removeComponent<Name>(e);
    reg_->removeComponent<Position>(e);
    reg_->removeComponent<PrimevalEssence>(e);
    entity_manager_.destroyEntity(e);
}
