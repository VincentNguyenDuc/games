#include "game.hpp"

#include "actions/combat.hpp"

using namespace ecse;
#include "actions/loot.hpp"
#include "components/ai_behavior.hpp"
#include "components/aperture.hpp"
#include "components/cultivation_rank.hpp"
#include "components/health.hpp"
#include "components/loot.hpp"
#include "components/move_intent.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/stats.hpp"
#include "input/parser.hpp"
#include "rendering/render.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>

using namespace ftxui;

Game::Game()
    : db_(std::make_unique<InMemoryGuWormDatabase>()) {}

void Game::run() {
    spawn_player();
    spawn_enemies();

    ai_sys_ = &ecs_engine_.add_system<AiTickSystem>(game_world_, player_);
    move_sys_ = &ecs_engine_.add_system<MoveTickSystem>(game_world_);
    self_eff_sys_ = &ecs_engine_.add_system<SelfEffectTickSystem>();
    atk_eff_sys_ = &ecs_engine_.add_system<AttackEffectTickSystem>();
    ecs_engine_.build();

    std::string input_text;
    auto input = Input(&input_text, "type command…");

    auto game_ui = Renderer(input, [&] {
        return vbox({
            render(ecs_engine_.registry(), game_world_, player_, status_msg_),
            separator(),
            hbox({text(" > "), input->Render()}) | border,
        });
    });

    game_ui = CatchEvent(game_ui, [&](Event event) -> bool {
        {
            std::optional<PlayerCommand> cmd;
            if (event == Event::ArrowUp)
                cmd = MoveCommand{"north"};
            else if (event == Event::ArrowDown)
                cmd = MoveCommand{"south"};
            else if (event == Event::ArrowRight)
                cmd = MoveCommand{"east"};
            else if (event == Event::ArrowLeft)
                cmd = MoveCommand{"west"};
            else if (event == Event::Character(' ') && input_text.empty())
                cmd = SkipCommand{};

            if (cmd) {
                process_command(*cmd);
                return true;
            }
        }

        if (event == Event::Return && !input_text.empty()) {
            if (auto cmd = parse_command(input_text))
                process_command(*cmd);
            else
                status_msg_ = "Unknown command.";
            input_text.clear();
            return true;
        }

        return false;
    });

    screen_.Loop(game_ui);
}

// ── Spawning ──────────────────────────────────────────────────────────────────

void Game::spawn_player() {
    player_ = ecs_engine_.entities().createEntity();
    auto strength_gu = db_->get("Strength Gu");

    auto& reg = ecs_engine_.registry();
    reg.addComponent(player_, Name{"Fang Yuan"});
    reg.addComponent(player_, Health{100, 100});
    reg.addComponent(player_, PrimevalEssence{60, 60, 0});
    reg.addComponent(player_, CultivationRank{1, 0});
    reg.addComponent(player_, Aperture{{{strength_gu}}, 3});
    reg.addComponent(player_, Position{game_world_.entrance_id(), 5, 5});

    game_world_.add_entity(game_world_.entrance_id(), player_);
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
    Entity e = ecs_engine_.entities().createEntity();
    auto& reg = ecs_engine_.registry();
    reg.addComponent(e, Name{name});
    reg.addComponent(e, Health{hp, hp});
    int range = (behavior == BehaviorType::Guardian)  ? 2
                : (behavior == BehaviorType::Schemer) ? 3
                                                      : 1;
    reg.addComponent(e, Stats{base_attack, base_defense, range});
    reg.addComponent(e, AIBehavior{behavior});

    std::vector<GuWorm> enemy_worms;
    for (const auto& d : drops)
        enemy_worms.push_back({d.def});
    reg.addComponent(e, Aperture{enemy_worms, (int)enemy_worms.size()});

    int essence = (behavior == BehaviorType::Guardian)  ? 80
                  : (behavior == BehaviorType::Schemer) ? 50
                                                        : 30;
    reg.addComponent(e, PrimevalEssence{essence, essence, 0});
    reg.addComponent(e, Loot{std::move(drops)});

    auto [ex, ey] = game_world_.random_empty_cell(map);
    reg.addComponent(e, Position{map, ex, ey});
    game_world_.add_entity(map, e);
    return e;
}

void Game::spawn_enemies() {
    auto strength_gu = db_->get("Strength Gu");
    auto iron_skin_gu = db_->get("Iron Skin Gu");
    auto lightning_gu = db_->get("Lightning Gu");
    auto jade_skin_gu = db_->get("Jade Skin Gu");
    auto vital_gu = db_->get("Vital Gu");
    auto thunder_gu = db_->get("Thunder Stomp Gu");

    make_enemy(1, "Wild Strength Gu", 18, 6, 0, BehaviorType::Wild, {{{strength_gu}, 0.80f}});
    make_enemy(2, "Wild Strength Gu", 18, 6, 0, BehaviorType::Wild, {{{strength_gu}, 0.60f}});
    make_enemy(2, "Wild Iron Skin Worm", 22, 4, 3, BehaviorType::Wild, {{{iron_skin_gu}, 0.60f}});
    make_enemy(
        3,
        "Demonic Gu Master - Wei",
        45,
        14,
        2,
        BehaviorType::Schemer,
        {{{lightning_gu}, 0.70f}, {{iron_skin_gu}, 0.50f}}
    );
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

    Map* cache = game_world_.get_map(5);
    cache->dropped_worms.push_back({db_->get("Moonlight Gu")});
    cache->dropped_worms.push_back({db_->get("Steel Bones Gu")});

    make_enemy(
        6,
        "Iron Guardian Construct",
        65,
        18,
        5,
        BehaviorType::Guardian,
        {{{db_->get("Rock Skin Gu")}, 1.0f}, {{thunder_gu}, 0.50f}}
    );
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

// ── Command handling ──────────────────────────────────────────────────────────

void Game::process_command(const PlayerCommand& cmd) {
    auto& reg = ecs_engine_.registry();

    std::visit(
        [this, &reg](const auto& c) {
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, QuitCommand>) {
            status_msg_ = "You abandon your path. The Grotto-Heaven swallows you whole.";
            screen_.Exit();
            return;
        }

        if constexpr (std::is_same_v<T, MoveCommand>) {
            int dx = 0, dy = 0;
            if (c.direction == "north")
                dy = -1;
            else if (c.direction == "south")
                dy = 1;
            else if (c.direction == "east")
                dx = 1;
            else if (c.direction == "west")
                dx = -1;
            else {
                status_msg_ = "Unknown direction.";
                return;
            }

            auto* pos_before = reg.getComponent<Position>(player_);
            int bx = pos_before->x, by = pos_before->y;
            MapId bmap = pos_before->map_id;

            reg.addComponent(player_, MoveIntentComponent{dx, dy, /*allow_slide=*/false});
            std::string move_msg = MoveTickSystem::resolve(reg, game_world_);

            auto* pos_after = reg.getComponent<Position>(player_);
            bool moved = (pos_after->x != bx || pos_after->y != by || pos_after->map_id != bmap);
            if (moved) {
                auto* essence = reg.getComponent<PrimevalEssence>(player_);
                int regen = std::max(1, essence->max / 5);
                essence->current = std::min(essence->max, essence->current + regen);
                essence->depleted_turns = 0;
                status_msg_ = move_msg;
            }
        }

        if constexpr (std::is_same_v<T, AttackCommand>) {
            auto* pos = reg.getComponent<Position>(player_);
            Map* map = game_world_.get_map(pos->map_id);

            std::vector<Entity> enemies;
            for (Entity e : map->entities)
                if (e != player_ && reg.getComponent<Health>(e))
                    enemies.push_back(e);

            if (enemies.empty()) {
                status_msg_ = "No enemies here.";
                return;
            }

            Entity target = enemies[0];
            auto result = activate_worm(reg, player_, target, c.worm_slot);
            status_msg_ = result.message;
            if (result.success)
                post_action_tick();
        }

        if constexpr (std::is_same_v<T, HealCommand>) {
            auto result = activate_worm(reg, player_, player_, c.worm_slot);
            status_msg_ = result.message;
            if (result.success)
                post_action_tick();
        }

        if constexpr (std::is_same_v<T, PickupCommand>) {
            pickup_worm(reg, game_world_, player_, c.pickup_index);
            status_msg_.clear();
        }

        if constexpr (std::is_same_v<T, DropCommand>) {
            drop_worm(reg, game_world_, player_, c.pickup_index);
            status_msg_.clear();
        }

        if constexpr (std::is_same_v<T, SkipCommand>) {
            auto* essence = reg.getComponent<PrimevalEssence>(player_);
            if (!is_in_combat()) {
                int regen = std::max(1, essence->max / 5);
                essence->current = std::min(essence->max, essence->current + regen);
                status_msg_ = "You focus and let your essence settle.";
            }
            post_action_tick();
        }
        },
        cmd);

    check_win();
}

void Game::post_action_tick() {
    if (!is_in_combat())
        return;

    ecs_engine_.tick();

    std::string msgs;
    msgs += ai_sys_->output;
    msgs += move_sys_->output;
    msgs += self_eff_sys_->output;
    msgs += atk_eff_sys_->output;

    if (!msgs.empty())
        status_msg_ += (status_msg_.empty() ? "" : "\n") + msgs;

    cleanup_dead();

    auto* essence = ecs_engine_.registry().getComponent<PrimevalEssence>(player_);
    if (essence->current == 0) {
        essence->depleted_turns++;
        if (essence->depleted_turns >= 3) {
            status_msg_ = "Your aperture has collapsed. DEFEAT.";
            screen_.Exit();
            return;
        }
        status_msg_ +=
            fmt::format("  WARNING: essence depleted ({}/3 turns)!", essence->depleted_turns);
    } else {
        essence->depleted_turns = 0;
    }
}

void Game::cleanup_dead() {
    auto& reg = ecs_engine_.registry();
    auto* player_hp = reg.getComponent<Health>(player_);
    if (player_hp->hp <= 0) {
        status_msg_ = "You have fallen. Your Gu worms scatter into the void. DEFEAT.";
        screen_.Exit();
        return;
    }

    auto* pos = reg.getComponent<Position>(player_);
    Map* map = game_world_.get_map(pos->map_id);
    std::vector<Entity> dead;
    for (Entity e : map->entities)
        if (e != player_ && reg.getComponent<Health>(e) && reg.getComponent<Health>(e)->hp <= 0)
            dead.push_back(e);

    for (Entity e : dead) {
        process_death(reg, game_world_, e);
        destroy_entity(e);
    }
}

bool Game::check_win() {
    auto& reg = ecs_engine_.registry();
    auto* pos = reg.getComponent<Position>(player_);
    Map* map = game_world_.get_map(pos->map_id);
    if (!map->is_exit)
        return false;

    for (Entity e : map->entities)
        if (e != player_ && reg.getComponent<Health>(e))
            return false;

    status_msg_ = "The exit seal shatters. You step through. VICTORY.";
    screen_.Exit();
    return true;
}

bool Game::is_in_combat() const {
    auto& reg = ecs_engine_.registry();
    auto* pos = reg.getComponent<Position>(player_);
    Map* map = game_world_.get_map(pos->map_id);
    for (Entity e : map->entities)
        if (e != player_ && reg.getComponent<AIBehavior>(e) && reg.getComponent<Health>(e))
            return true;
    return false;
}

void Game::destroy_entity(Entity e) {
    auto& reg = ecs_engine_.registry();
    auto* pos = reg.getComponent<Position>(e);
    if (pos)
        game_world_.remove_entity(pos->map_id, e);

    reg.removeComponent<Health>(e);
    reg.removeComponent<Stats>(e);
    reg.removeComponent<AIBehavior>(e);
    reg.removeComponent<Loot>(e);
    reg.removeComponent<Aperture>(e);
    reg.removeComponent<Name>(e);
    reg.removeComponent<Position>(e);
    reg.removeComponent<PrimevalEssence>(e);
    ecs_engine_.entities().destroyEntity(e);
}
