#include "game.hpp"

#include "actions.hpp"
#include "input.hpp"
#include "persistence/save_load.hpp"
#include "renderer.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <raylib.h>
#include <spdlog/spdlog.h>

// ─── Construction ────────────────────────────────────────────────────────────

Game::Game() {
    setup_ecs();

    actions_ = std::make_unique<Actions>(*this);
    renderer_ = std::make_unique<Renderer>(*this);
    input_ = std::make_unique<InputHandler>(*this);

    // Wire ECS callbacks — lambdas capture 'this'; actions_ is valid at call time
    build_sys_->on_complete = [this](ecse::Entity e, bool is_upgrade) {
        actions_->finish_construction(e, is_upgrade);
    };
    extract_sys_->on_cleared = [this](ecse::Entity e) { actions_->finish_obstacle(e); };
    xp_sys_->on_level_up = [this](int lv) { actions_->on_level_up(lv); };

    actions_->spawn_initial_buildings();

    last_tick_ = std::chrono::steady_clock::now();

    if (std::filesystem::exists(Game::DB_PATH))
        load_game();
}

Game::~Game() = default;

// ─── ECS setup ───────────────────────────────────────────────────────────────

void Game::setup_ecs() {
    player_ = ecs_.entities().createEntity();
    ecs_.registry().addComponent<Player>(player_, {});
    ecs_.registry().addComponent<Experience>(player_, {});
    ecs_.registry().addComponent<Position>(player_, {1, 1});

    produce_sys_ = &ecs_.add_system<ProduceSystem>();
    build_sys_ = &ecs_.add_system<ConstructionSystem>();
    boost_sys_ = &ecs_.add_system<BoostSystem>();
    xp_sys_ = &ecs_.add_system<LevelUpSystem>();
    extract_sys_ = &ecs_.add_system<ExtractSystem>();

    xp_sys_->player_entity = player_;

    ecs_.build();
}

// ─── Game loop ────────────────────────────────────────────────────────────────

void Game::run() {
    InitWindow(WINDOW_W, WINDOW_H, "Leah's Village");
    SetTargetFPS(60);
    last_tick_ = std::chrono::steady_clock::now();

    while (!WindowShouldClose() && !quit_) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_tick_).count();
        last_tick_ = now;

        input_->handle_input();
        tick(dt);

        BeginDrawing();
        ClearBackground({15, 15, 25, 255});
        renderer_->render();
        EndDrawing();
    }

    save_game();
    CloseWindow();
}

// ─── Tick ─────────────────────────────────────────────────────────────────────

void Game::tick(float dt) {
    produce_sys_->dt = dt;
    build_sys_->dt = dt;
    boost_sys_->dt = dt;
    extract_sys_->dt = dt;
    ecs_.tick();
}

// ─── Resource helpers ────────────────────────────────────────────────────────

int Game::total_gold() const {
    int sum = 0;
    for (ecse::Entity e : ecs_.registry().view<ResourceStorage>()) {
        auto* s = ecs_.registry().getComponent<ResourceStorage>(e);
        if (s && s->resource == ResourceType::Gold)
            sum += s->current;
    }
    return sum;
}

int Game::total_gold_cap() const {
    int sum = 0;
    for (ecse::Entity e : ecs_.registry().view<ResourceStorage>()) {
        auto* s = ecs_.registry().getComponent<ResourceStorage>(e);
        if (s && s->resource == ResourceType::Gold)
            sum += s->capacity;
    }
    return sum;
}

int Game::total_elixir() const {
    int sum = 0;
    for (ecse::Entity e : ecs_.registry().view<ResourceStorage>()) {
        auto* s = ecs_.registry().getComponent<ResourceStorage>(e);
        if (s && s->resource == ResourceType::Elixir)
            sum += s->current;
    }
    return sum;
}

int Game::total_elixir_cap() const {
    int sum = 0;
    for (ecse::Entity e : ecs_.registry().view<ResourceStorage>()) {
        auto* s = ecs_.registry().getComponent<ResourceStorage>(e);
        if (s && s->resource == ResourceType::Elixir)
            sum += s->capacity;
    }
    return sum;
}

bool Game::spend_resources(int gold, int elixir) {
    if (total_gold() < gold || total_elixir() < elixir)
        return false;
    add_resources(-gold, -elixir);
    return true;
}

void Game::add_resources(int gold, int elixir) {
    auto distribute = [&](ResourceType rtype, int amount) {
        for (ecse::Entity e : ecs_.registry().view<ResourceStorage>()) {
            if (amount == 0)
                break;
            auto* s = ecs_.registry().getComponent<ResourceStorage>(e);
            if (!s || s->resource != rtype)
                continue;
            int delta = (amount > 0) ? std::min(amount, s->capacity - s->current)
                                     : std::max(amount, -s->current);
            s->current += delta;
            amount -= delta;
        }
    };
    distribute(ResourceType::Gold, gold);
    distribute(ResourceType::Elixir, elixir);
}

void Game::grant_xp(int amount) {
    if (auto* existing = ecs_.registry().getComponent<PendingXP>(player_))
        existing->amount += amount;
    else
        ecs_.registry().addComponent<PendingXP>(player_, {amount});
}

int Game::free_builders() const {
    auto* p = ecs_.registry().getComponent<Player>(player_);
    return p ? (p->builder_slots - p->busy_builders) : 0;
}

// ─── Persistence ─────────────────────────────────────────────────────────────

void Game::save_game() {
    Persistence::save(Game::DB_PATH, *this);
    push_msg("Game saved.");
}

void Game::load_game() {
    if (Persistence::load(Game::DB_PATH, *this))
        push_msg("Game loaded.");
    else
        push_msg("No save found.");
}

// ─── Utility ─────────────────────────────────────────────────────────────────

void Game::push_msg(std::string msg) {
    spdlog::info("{}", msg);
    messages_.push_back(std::move(msg));
    while (static_cast<int>(messages_.size()) > MAX_MSG)
        messages_.pop_front();
}

std::string Game::fmt_time(float seconds) {
    int s = static_cast<int>(seconds);
    if (s < 60)
        return fmt::format("{}s", s);
    if (s < 3600)
        return fmt::format("{}m{}s", s / 60, s % 60);
    return fmt::format("{}h{}m", s / 3600, (s % 3600) / 60);
}
