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

#include <chrono>
#include <deque>
#include <string>

class Game {
public:
    inline static const std::string DB_PATH = "leah_village/game.db";

    Game();
    void run();

    // --- Persistence interface ---
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
    void grant_xp(int amount);

private:
    // ── Window / layout constants ─────────────────────────────────────────────
    static constexpr int WINDOW_W = 1280;
    static constexpr int WINDOW_H = 720;
    static constexpr int TILE_W = 40;
    static constexpr int TILE_H = 34;
    static constexpr int VIEWPORT_COLS = 20;
    static constexpr int VIEWPORT_ROWS = 12;
    static constexpr int MAP_PX_W = TILE_W * VIEWPORT_COLS; // 800
    static constexpr int MAP_PX_H = TILE_H * VIEWPORT_ROWS; // 408
    static constexpr int HUD_H = 60;
    static constexpr int MAP_Y = HUD_H;
    static constexpr int PANEL_X = MAP_PX_W;
    static constexpr int PANEL_W = WINDOW_W - MAP_PX_W;  // 480
    static constexpr int STATUS_Y = MAP_Y + MAP_PX_H;    // 468
    static constexpr int STATUS_H = WINDOW_H - STATUS_Y; // 252

    // ── Core state ────────────────────────────────────────────────────────────
    World world_;
    ecse::Engine ecs_;
    ecse::Entity player_{0};
    ecse::Entity selected_{0};

    ProduceSystem* produce_sys_{nullptr};
    ConstructionSystem* build_sys_{nullptr};
    BoostSystem* boost_sys_{nullptr};
    LevelUpSystem* xp_sys_{nullptr};
    ExtractSystem* extract_sys_{nullptr};

    std::chrono::steady_clock::time_point last_tick_;
    bool quit_{false};

    // ── UI state ──────────────────────────────────────────────────────────────
    enum class Mode { Normal, BuildSelect } mode_{Mode::Normal};
    BuildingType build_cursor_type_{BuildingType::GoldMine};
    int build_menu_idx_{0};

    std::deque<std::string> messages_;
    static constexpr int MAX_MSG = 6;

    // ── Initialisation ────────────────────────────────────────────────────────
    void setup_ecs();
    void spawn_initial_buildings();
    ecse::Entity spawn_building(MapId map_id, int x, int y, BuildingType type, int level = 1);
    ecse::Entity spawn_obstacle(
        MapId map_id, int x, int y, int gold_reward, int elixir_reward, float clear_time_s
    );

    // ── Game tick ─────────────────────────────────────────────────────────────
    void tick(float dt);

    // ── Player actions ────────────────────────────────────────────────────────
    void try_move(int dx, int dy);
    void try_interact();
    void try_upgrade();
    void try_clear();
    void enter_build_mode();
    void confirm_build();
    void cancel_build();

    void do_collect(ecse::Entity e);
    void start_construction(ecse::Entity e, ConstructionKind kind, float time_s);
    void finish_construction(ecse::Entity e, bool is_upgrade);
    void do_clear_obstacle(ecse::Entity e);
    void finish_obstacle(ecse::Entity e);
    void on_level_up(int new_level);

    // ── Rendering (raylib — draw directly, return void) ───────────────────────
    void render();
    void render_hud();
    void render_map();
    void render_cell(int x, int y, int px, int py);
    void render_panel();
    void render_statusbar();

    // ── Input (polls raylib key state) ────────────────────────────────────────
    void handle_input();

    // ── Persistence ───────────────────────────────────────────────────────────
    void save_game();
    void load_game();

    // ── Utility ───────────────────────────────────────────────────────────────
    void push_msg(std::string msg);
    int free_builders() const;
    static std::string fmt_time(float seconds);
};
