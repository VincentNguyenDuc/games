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
#include <memory>
#include <string>

// Forward declarations — full types are only needed in the .cpp files
class Renderer;
class Actions;
class InputHandler;

class Game {
    friend class Renderer;
    friend class Actions;
    friend class InputHandler;

public:
    inline static const std::string DB_PATH = "leah_village/game.db";

    Game();
    ~Game();
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
    static constexpr int MAP_PX_W = TILE_W * VIEWPORT_COLS;
    static constexpr int MAP_PX_H = TILE_H * VIEWPORT_ROWS;
    static constexpr int HUD_H = 60;
    static constexpr int MAP_Y = HUD_H;
    static constexpr int PANEL_X = MAP_PX_W;
    static constexpr int PANEL_W = WINDOW_W - MAP_PX_W;
    static constexpr int STATUS_Y = MAP_Y + MAP_PX_H;
    static constexpr int STATUS_H = WINDOW_H - STATUS_Y;

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

    // ── Subsystems ────────────────────────────────────────────────────────────
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<Actions> actions_;
    std::unique_ptr<InputHandler> input_;

    // ── Internal methods ──────────────────────────────────────────────────────
    void setup_ecs();
    void tick(float dt);
    void save_game();
    void load_game();
    void push_msg(std::string msg);
    static std::string fmt_time(float seconds);
    int free_builders() const;
};
