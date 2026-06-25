#pragma once

#include "components/boost.hpp"
#include "components/builder.hpp"
#include "components/building.hpp"
#include "components/construction.hpp"
#include "components/experience.hpp"
#include "components/level.hpp"
#include "components/locked.hpp"
#include "components/map_location.hpp"
#include "components/name.hpp"
#include "components/obstacle.hpp"
#include "components/pending_xp.hpp"
#include "components/player.hpp"
#include "components/population.hpp"
#include "components/position.hpp"
#include "components/resource_producer.hpp"
#include "components/resource_storage.hpp"
#include "components/selected.hpp"
#include "components/upgrade_cost.hpp"
#include "ecs.hpp"
#include "systems/boost_system.hpp"
#include "systems/build_system.hpp"
#include "systems/extract_system.hpp"
#include "systems/level_up_system.hpp"
#include "systems/produce_system.hpp"
#include "world/world.hpp"

#include <atomic>
#include <deque>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <thread>

class Game {
public:
    Game();
    void run();

    // --- Persistence interface (used by Persistence::load / ::save) ---
    ecse::Engine& ecs() { return ecs_; }
    const ecse::Engine& ecs() const { return ecs_; }
    World& world() { return world_; }
    const World& world() const { return world_; }
    ecse::Entity player_entity() const { return player_; }
    ecse::Entity selected_entity() const { return selected_; }
    void set_player_entity(ecse::Entity e) { player_ = e; }

    // Resource helpers used externally
    int total_gold() const;
    int total_gold_cap() const;
    int total_elixir() const;
    int total_elixir_cap() const;
    bool spend_resources(int gold, int elixir);
    void add_resources(int gold, int elixir);

    // Award XP to the player (fires through ECS next tick)
    void grant_xp(int amount);

private:
    // --- Core state ---
    World world_;
    ecse::Engine ecs_;
    ecse::Entity player_{0};
    ecse::Entity selected_{0}; // 0 = nothing selected

    // System pointers (owned by ecs_, kept for dt injection)
    ProduceSystem* produce_sys_{nullptr};
    ConstructionSystem* build_sys_{nullptr};
    BoostSystem* boost_sys_{nullptr};
    LevelUpSystem* xp_sys_{nullptr};
    ExtractSystem* extract_sys_{nullptr};

    // --- Tick timing ---
    std::chrono::steady_clock::time_point last_tick_;

    // --- UI state ---
    enum class Mode { Normal, BuildSelect } mode_{Mode::Normal};
    BuildingType build_cursor_type_{BuildingType::GoldMine};
    int build_menu_idx_{0};

    std::deque<std::string> messages_; // last 4 shown in HUD
    static constexpr int MAX_MSG = 4;

    std::atomic<bool> running_{true};
    ftxui::ScreenInteractive screen_ = ftxui::ScreenInteractive::Fullscreen();

    // --- Initialisation ---
    void setup_ecs();
    void spawn_initial_buildings();
    ecse::Entity spawn_building(MapId map_id, int x, int y, BuildingType type, int level = 1);
    ecse::Entity spawn_obstacle(
        MapId map_id, int x, int y, int gold_reward, int elixir_reward, float clear_time_s
    );

    // --- Game tick (called from main thread) ---
    void tick(float dt);

    // --- Player actions ---
    void try_move(int dx, int dy);
    void try_interact();     // E: collect or select building
    void try_upgrade();      // U: upgrade selected building
    void try_clear();        // C: start clearing selected obstacle
    void enter_build_mode(); // B: open build menu
    void confirm_build();    // Enter in build mode
    void cancel_build();     // Esc in build mode

    void do_collect(ecse::Entity e);
    void start_construction(ecse::Entity e, ConstructionKind kind, float time_s);
    void finish_construction(ecse::Entity e, bool is_upgrade);
    void do_clear_obstacle(ecse::Entity e);
    void finish_obstacle(ecse::Entity e);
    void on_level_up(int new_level);

    // --- Rendering (FTXUI) ---
    ftxui::Element render();
    ftxui::Element render_hud();
    ftxui::Element render_map();
    ftxui::Element render_cell(int x, int y);
    ftxui::Element render_panel();
    ftxui::Element render_statusbar();

    bool handle_input(ftxui::Event e);

    // --- Persistence ---
    void save_game();
    void load_game();

    // --- Utility ---
    void push_msg(std::string msg);
    int free_builders() const;
    static std::string fmt_time(float seconds);
};
