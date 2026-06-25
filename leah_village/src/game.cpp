#include "game.hpp"

#include "assets.hpp"
#include "persistence/save_load.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <spdlog/spdlog.h>
#include <thread>

using namespace ftxui;

// ─── Construction ────────────────────────────────────────────────────────────

Game::Game() {
    setup_ecs();
    spawn_initial_buildings();

    last_tick_ = std::chrono::steady_clock::now();

    // Try to resume a saved game; if none exists, start fresh.
    if (std::filesystem::exists("leah_village.db"))
        load_game();
}

// ─── ECS setup ───────────────────────────────────────────────────────────────

void Game::setup_ecs() {
    // Create player entity
    player_ = ecs_.entities().createEntity();
    ecs_.registry().addComponent<Player>(player_, {});
    ecs_.registry().addComponent<Experience>(player_, {});
    ecs_.registry().addComponent<Position>(player_, {1, 1});

    // Register systems
    produce_sys_ = &ecs_.add_system<ProduceSystem>();
    build_sys_ = &ecs_.add_system<ConstructionSystem>();
    boost_sys_ = &ecs_.add_system<BoostSystem>();
    xp_sys_ = &ecs_.add_system<LevelUpSystem>();
    extract_sys_ = &ecs_.add_system<ExtractSystem>();

    build_sys_->on_complete = [this](ecse::Entity e, bool is_upgrade) {
        finish_construction(e, is_upgrade);
    };
    extract_sys_->on_cleared = [this](ecse::Entity e) { finish_obstacle(e); };
    xp_sys_->player_entity = player_;
    xp_sys_->on_level_up = [this](int lv) { on_level_up(lv); };

    ecs_.build();
}

// ─── Entity spawning ─────────────────────────────────────────────────────────

ecse::Entity Game::spawn_building(MapId map_id, int x, int y, BuildingType type, int level) {
    const auto& d = Assets::def(type);
    const auto& stat = d.stats[level - 1];

    ecse::Entity e = ecs_.entities().createEntity();
    ecs_.registry().addComponent<Position>(e, {x, y});
    ecs_.registry().addComponent<MapLocation>(e, {map_id});
    ecs_.registry().addComponent<Name>(e, {std::string(d.name)});
    ecs_.registry().addComponent<Building>(e, {type, 1, stat.hp, stat.hp});
    ecs_.registry().addComponent<Level>(e, {level, d.max_level});

    // Attach subsystem components based on type
    switch (type) {
    case BuildingType::GoldMine:
        ecs_.registry().addComponent<ResourceProducer>(
            e, {ResourceType::Gold, stat.rate_per_hour, 0.f, stat.prod_cap}
        );
        break;
    case BuildingType::ElixirCollector:
        ecs_.registry().addComponent<ResourceProducer>(
            e, {ResourceType::Elixir, stat.rate_per_hour, 0.f, stat.prod_cap}
        );
        break;
    case BuildingType::GoldStorage:
        ecs_.registry().addComponent<ResourceStorage>(e, {ResourceType::Gold, 0, stat.storage_cap});
        break;
    case BuildingType::ElixirStorage:
        ecs_.registry().addComponent<ResourceStorage>(
            e, {ResourceType::Elixir, 0, stat.storage_cap}
        );
        break;
    case BuildingType::BuilderHut:
        ecs_.registry().addComponent<Builder>(e, {false, 0});
        break;
    case BuildingType::Farm:
        ecs_.registry().addComponent<PopulationProvider>(e, {stat.pop_cap});
        break;
    default:
        break;
    }

    // Attach upgrade cost for next level (if not already at max)
    if (level < d.max_level) {
        const auto& next = d.stats[level];
        ecs_.registry().addComponent<UpgradeCost>(e, {next.gold_cost, next.elixir_cost});
    }

    Map& m = world_.get_map(map_id);
    m.place_entity(x, y, e);
    return e;
}

ecse::Entity Game::spawn_obstacle(
    MapId map_id, int x, int y, int gold, int elixir, float clear_time_s
) {
    ecse::Entity e = ecs_.entities().createEntity();
    ecs_.registry().addComponent<Position>(e, {x, y});
    ecs_.registry().addComponent<MapLocation>(e, {map_id});
    ecs_.registry().addComponent<Obstacle>(e, {gold, elixir, clear_time_s, 0.f});

    world_.get_map(map_id).place_entity(x, y, e);
    return e;
}

void Game::spawn_initial_buildings() {
    // ── Home Village (Map 0) ──────────────────────────────────────────────────
    spawn_building(0, 10, 5, BuildingType::TownHall);
    spawn_building(0, 4, 3, BuildingType::GoldMine);
    spawn_building(0, 16, 3, BuildingType::ElixirCollector);
    spawn_building(0, 4, 9, BuildingType::GoldStorage);
    spawn_building(0, 16, 9, BuildingType::ElixirStorage);
    spawn_building(0, 2, 9, BuildingType::BuilderHut);
    spawn_building(0, 18, 9, BuildingType::Farm);

    spawn_obstacle(0, 7, 2, 25, 0, 45.f);
    spawn_obstacle(0, 13, 2, 0, 25, 45.f);
    spawn_obstacle(0, 7, 8, 30, 0, 60.f);
    spawn_obstacle(0, 13, 8, 0, 30, 60.f);

    // ── Ancient Forest (Map 1) ───────────────────────────────────────────────
    spawn_building(1, 5, 5, BuildingType::ElixirCollector);
    spawn_building(1, 13, 5, BuildingType::ElixirCollector);
    spawn_building(1, 9, 8, BuildingType::Farm);

    for (auto [ox, oy] : std::initializer_list<std::pair<int, int>>{
             {2, 2}, {3, 7}, {7, 3}, {11, 8}, {15, 2}, {15, 7}, {6, 9}, {12, 3}})
        spawn_obstacle(1, ox, oy, 0, 40, 90.f);

    // ── Crystal Caves (Map 2) ────────────────────────────────────────────────
    spawn_building(2, 6, 4, BuildingType::GoldMine, 2);
    spawn_building(2, 18, 4, BuildingType::GoldMine, 2);
    spawn_building(2, 12, 8, BuildingType::GoldStorage, 2);
    spawn_building(2, 4, 11, BuildingType::Farm);

    for (auto [ox, oy] : std::initializer_list<std::pair<int, int>>{
             {9, 3}, {14, 3}, {3, 6}, {21, 6}, {9, 12}, {14, 12}})
        spawn_obstacle(2, ox, oy, 100, 0, 120.f);
}

// ─── Game loop ────────────────────────────────────────────────────────────────

void Game::run() {
    auto renderer = Renderer([this] { return render(); });
    auto component = CatchEvent(renderer, [this](Event e) { return handle_input(e); });

    // Timer thread: posts a Custom event every 200 ms → triggers tick + re-render.
    std::thread timer([this] {
        while (running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            screen_.PostEvent(Event::Custom);
        }
    });

    screen_.Loop(component);
    running_.store(false);
    timer.join();
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
    // Distribute across storages (gold first, then elixir).
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
    // Attach PendingXP to the player entity so LevelUpSystem picks it up next tick.
    if (auto* existing = ecs_.registry().getComponent<PendingXP>(player_))
        existing->amount += amount;
    else
        ecs_.registry().addComponent<PendingXP>(player_, {amount});
}

int Game::free_builders() const {
    auto* p = ecs_.registry().getComponent<Player>(player_);
    return p ? (p->builder_slots - p->busy_builders) : 0;
}

// ─── Player actions ───────────────────────────────────────────────────────────

void Game::try_move(int dx, int dy) {
    auto* pos = ecs_.registry().getComponent<Position>(player_);
    auto* pl = ecs_.registry().getComponent<Player>(player_);
    if (!pos || !pl)
        return;

    int nx = pos->x + dx;
    int ny = pos->y + dy;
    Map& m = world_.get_map(pl->current_map_id);

    if (!m.in_bounds(nx, ny))
        return;

    int cell = m.grid[ny][nx];

    if (cell == Cell::Gate) {
        auto it = m.gates.find(m.cell_key(nx, ny));
        if (it == m.gates.end())
            return;
        const Gate& g = it->second;
        Map& target = world_.get_map(g.target_map);

        auto* exp = ecs_.registry().getComponent<Experience>(player_);
        if (exp && exp->level < target.required_player_level) {
            push_msg(
                fmt::format("Need level {} to enter {}.", target.required_player_level, target.name)
            );
            return;
        }

        pl->current_map_id = g.target_map;
        pos->x = g.target_x;
        pos->y = g.target_y;

        // Award first-visit XP
        if (g.target_map == 1 && !pl->visited_forest) {
            pl->visited_forest = true;
            grant_xp(50);
            push_msg(fmt::format("Entered {}! +50 XP", target.name));
        } else if (g.target_map == 2 && !pl->visited_caves) {
            pl->visited_caves = true;
            grant_xp(100);
            push_msg(fmt::format("Entered {}! +100 XP", target.name));
        } else {
            push_msg(fmt::format("Entered {}.", target.name));
        }
        world_.travel_to(g.target_map);
        selected_ = 0;
        return;
    }

    if (m.walkable(nx, ny)) {
        pos->x = nx;
        pos->y = ny;
        // Auto-select whatever is adjacent (deselect if open ground)
        selected_ = 0;
    }
}

void Game::try_interact() {
    auto* pos = ecs_.registry().getComponent<Position>(player_);
    auto* pl = ecs_.registry().getComponent<Player>(player_);
    if (!pos || !pl)
        return;

    Map& m = world_.current_map();
    ecse::Entity e = m.get_entity(pos->x, pos->y);

    // Check 4-directional adjacency too
    if (!e) {
        for (auto [dx, dy] :
             std::initializer_list<std::pair<int, int>>{{0, -1}, {0, 1}, {-1, 0}, {1, 0}}) {
            ecse::Entity adj = m.get_entity(pos->x + dx, pos->y + dy);
            if (adj) {
                e = adj;
                break;
            }
        }
    }

    if (!e) {
        selected_ = 0;
        return;
    }

    selected_ = e;

    // If it's a ResourceProducer with something stored → collect
    if (auto* prod = ecs_.registry().getComponent<ResourceProducer>(e)) {
        if (prod->stored >= 1.f) {
            do_collect(e);
            return;
        }
    }

    // If it's an Obstacle → start clearing
    if (auto* obs = ecs_.registry().getComponent<Obstacle>(e)) {
        if (obs->time_remaining_s <= 0.f)
            do_clear_obstacle(e);
        return;
    }
}

void Game::do_collect(ecse::Entity e) {
    auto* prod = ecs_.registry().getComponent<ResourceProducer>(e);
    if (!prod || prod->stored < 1.f)
        return;

    int amount = static_cast<int>(prod->stored);
    if (prod->resource == ResourceType::Gold) {
        int cap = total_gold_cap() - total_gold();
        amount = std::min(amount, cap);
        add_resources(amount, 0);
    } else {
        int cap = total_elixir_cap() - total_elixir();
        amount = std::min(amount, cap);
        add_resources(0, amount);
    }
    prod->stored -= static_cast<float>(amount);
    grant_xp(amount / 50);
    push_msg(fmt::format(
        "Collected {} {}.", amount, prod->resource == ResourceType::Gold ? "gold" : "elixir"
    ));
}

void Game::try_upgrade() {
    if (!selected_)
        return;
    ecse::Entity e = selected_;

    auto* lv = ecs_.registry().getComponent<Level>(e);
    auto* cost = ecs_.registry().getComponent<UpgradeCost>(e);
    auto* bld = ecs_.registry().getComponent<Building>(e);
    if (!lv || !cost || !bld) {
        push_msg("Can't upgrade this.");
        return;
    }
    if (ecs_.registry().getComponent<Construction>(e)) {
        push_msg("Already under construction.");
        return;
    }
    if (free_builders() <= 0) {
        push_msg("No free builders!");
        return;
    }
    if (!spend_resources(cost->gold, cost->elixir)) {
        push_msg("Not enough resources.");
        return;
    }

    const auto& d = Assets::def(bld->type);
    float time_s = static_cast<float>(d.stats[lv->level].build_time_s);
    start_construction(e, ConstructionKind::Upgrade, time_s);
    push_msg(fmt::format("Upgrading {} to lv{}… ({})", d.name, lv->level + 1, fmt_time(time_s)));
}

void Game::try_clear() {
    if (!selected_)
        return;
    ecse::Entity e = selected_;
    auto* obs = ecs_.registry().getComponent<Obstacle>(e);
    if (!obs) {
        push_msg("Nothing to clear.");
        return;
    }
    if (obs->time_remaining_s > 0.f) {
        push_msg("Already clearing.");
        return;
    }
    if (free_builders() <= 0) {
        push_msg("No free builders!");
        return;
    }
    do_clear_obstacle(e);
}

void Game::do_clear_obstacle(ecse::Entity e) {
    auto* obs = ecs_.registry().getComponent<Obstacle>(e);
    if (!obs)
        return;
    obs->time_remaining_s = obs->clear_time_s;
    auto* p = ecs_.registry().getComponent<Player>(player_);
    if (p)
        p->busy_builders++;
    push_msg(fmt::format("Clearing obstacle… ({})", fmt_time(obs->clear_time_s)));
}

void Game::finish_obstacle(ecse::Entity e) {
    auto* obs = ecs_.registry().getComponent<Obstacle>(e);
    auto* pos = ecs_.registry().getComponent<Position>(e);
    auto* ml = ecs_.registry().getComponent<MapLocation>(e);
    if (!obs || !pos || !ml)
        return;

    add_resources(obs->gold_reward, obs->elixir_reward);
    grant_xp(25);
    push_msg(fmt::format("Obstacle cleared! +{}G +{}E +25 XP", obs->gold_reward, obs->elixir_reward)
    );

    world_.get_map(ml->map_id).remove_entity_at(pos->x, pos->y);
    if (selected_ == e)
        selected_ = 0;

    auto* p = ecs_.registry().getComponent<Player>(player_);
    if (p && p->busy_builders > 0)
        p->busy_builders--;

    ecs_.entities().destroyEntity(e);
    ecs_.registry().removeComponent<Position>(e);
    ecs_.registry().removeComponent<MapLocation>(e);
    ecs_.registry().removeComponent<Obstacle>(e);
}

void Game::start_construction(ecse::Entity e, ConstructionKind kind, float time_s) {
    ecs_.registry().addComponent<Construction>(e, {kind, 0, time_s});
    auto* p = ecs_.registry().getComponent<Player>(player_);
    if (p)
        p->busy_builders++;
    // Remove upgrade cost while building
    ecs_.registry().removeComponent<UpgradeCost>(e);
}

void Game::finish_construction(ecse::Entity e, bool is_upgrade) {
    auto* bld = ecs_.registry().getComponent<Building>(e);
    auto* lv = ecs_.registry().getComponent<Level>(e);
    if (!bld || !lv)
        return;

    if (is_upgrade) {
        lv->level++;
        const auto& d = Assets::def(bld->type);
        const auto& stat = d.stats[lv->level - 1];
        bld->max_hp = stat.hp;
        bld->hp = stat.hp;

        // Update subsystem components with new stats
        if (auto* prod = ecs_.registry().getComponent<ResourceProducer>(e)) {
            prod->rate_per_hour = stat.rate_per_hour;
            prod->capacity = stat.prod_cap;
        }
        if (auto* stor = ecs_.registry().getComponent<ResourceStorage>(e)) {
            stor->capacity = stat.storage_cap;
        }
        if (auto* pop = ecs_.registry().getComponent<PopulationProvider>(e)) {
            pop->capacity = stat.pop_cap;
        }

        // Reattach upgrade cost for next level if applicable
        if (lv->level < d.max_level) {
            const auto& next = d.stats[lv->level];
            ecs_.registry().addComponent<UpgradeCost>(e, {next.gold_cost, next.elixir_cost});
        }

        int xp_award = 30 * lv->level;
        grant_xp(xp_award);
        push_msg(fmt::format("{} upgraded to lv{}! +{} XP", d.name, lv->level, xp_award));
    } else {
        grant_xp(20);
        auto* nm = ecs_.registry().getComponent<Name>(e);
        push_msg(fmt::format("{} built! +20 XP", nm ? nm->value : "Building"));
    }

    auto* p = ecs_.registry().getComponent<Player>(player_);
    if (p && p->busy_builders > 0)
        p->busy_builders--;
}

void Game::enter_build_mode() {
    mode_ = Mode::BuildSelect;
    build_menu_idx_ = 0;
    push_msg("Build mode: select type with +/-, place with Enter, cancel with Esc.");
}

void Game::confirm_build() {
    auto* pos = ecs_.registry().getComponent<Position>(player_);
    auto* pl = ecs_.registry().getComponent<Player>(player_);
    if (!pos || !pl)
        return;

    Map& m = world_.current_map();
    if (!m.in_bounds(pos->x, pos->y) || m.has_entity(pos->x, pos->y) ||
        m.grid[pos->y][pos->x] != Cell::Empty) {
        push_msg("Can't build here.");
        return;
    }
    if (free_builders() <= 0) {
        push_msg("No free builders!");
        return;
    }

    const auto& d = Assets::def(build_cursor_type_);
    const auto& stat = d.stats[0];

    if (!spend_resources(stat.gold_cost, stat.elixir_cost)) {
        push_msg("Not enough resources.");
        return;
    }

    ecse::Entity e = spawn_building(pl->current_map_id, pos->x, pos->y, build_cursor_type_, 1);
    start_construction(e, ConstructionKind::Build, static_cast<float>(stat.build_time_s));

    mode_ = Mode::Normal;
    selected_ = e;
    push_msg(fmt::format("Building {}… ({})", d.name, fmt_time(stat.build_time_s)));
}

void Game::cancel_build() {
    mode_ = Mode::Normal;
    push_msg("Build cancelled.");
}

void Game::on_level_up(int new_level) {
    auto* pl = ecs_.registry().getComponent<Player>(player_);
    if (pl)
        pl->builder_slots = Assets::builder_slots_for_level(new_level);

    std::string bonus;
    if (new_level == 3)
        bonus = " Ancient Forest unlocked!";
    else if (new_level == 5)
        bonus = " +1 builder slot!";
    else if (new_level == 7)
        bonus = " Crystal Caves unlocked!";
    else if (new_level == 10)
        bonus = " +1 builder slot!";

    push_msg(fmt::format("Level up! Now level {}.{}", new_level, bonus));
}

// ─── Persistence ─────────────────────────────────────────────────────────────

void Game::save_game() {
    Persistence::save("leah_village.db", *this);
    push_msg("Game saved.");
}

void Game::load_game() {
    if (Persistence::load("leah_village.db", *this))
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

// ─── Input ────────────────────────────────────────────────────────────────────

bool Game::handle_input(Event e) {
    // Timer tick
    if (e == Event::Custom) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_tick_).count();
        last_tick_ = now;
        tick(dt);
        return false; // still re-render
    }

    if (mode_ == Mode::BuildSelect) {
        static const BuildingType BUILD_TYPES[] = {
            BuildingType::GoldMine,
            BuildingType::ElixirCollector,
            BuildingType::GoldStorage,
            BuildingType::ElixirStorage,
            BuildingType::BuilderHut,
            BuildingType::Farm,
            BuildingType::Decoration,
        };
        constexpr int N = 7;

        if (e == Event::Character('+') || e == Event::ArrowRight) {
            build_menu_idx_ = (build_menu_idx_ + 1) % N;
            build_cursor_type_ = BUILD_TYPES[build_menu_idx_];
            return true;
        }
        if (e == Event::Character('-') || e == Event::ArrowLeft) {
            build_menu_idx_ = (build_menu_idx_ + N - 1) % N;
            build_cursor_type_ = BUILD_TYPES[build_menu_idx_];
            return true;
        }
        if (e == Event::Return) {
            confirm_build();
            return true;
        }
        if (e == Event::Escape) {
            cancel_build();
            return true;
        }
        // Also allow WASD movement in build mode (to position player on target tile)
    }

    // Movement
    if (e == Event::Character('w') || e == Event::ArrowUp) {
        try_move(0, -1);
        return true;
    }
    if (e == Event::Character('s') || e == Event::ArrowDown) {
        try_move(0, 1);
        return true;
    }
    if (e == Event::Character('a') || e == Event::ArrowLeft) {
        try_move(-1, 0);
        return true;
    }
    if (e == Event::Character('d') || e == Event::ArrowRight) {
        try_move(1, 0);
        return true;
    }

    // Actions
    if (e == Event::Character('e') || e == Event::Character('E')) {
        try_interact();
        return true;
    }
    if (e == Event::Character('u') || e == Event::Character('U')) {
        try_upgrade();
        return true;
    }
    if (e == Event::Character('c') || e == Event::Character('C')) {
        try_clear();
        return true;
    }
    if (e == Event::Character('b') || e == Event::Character('B')) {
        enter_build_mode();
        return true;
    }

    // Save / Load
    if (e == Event::Character('\x13')) {
        save_game();
        return true;
    } // Ctrl+S
    if (e == Event::Character('\f')) {
        load_game();
        return true;
    } // Ctrl+L

    // Quit
    if (e == Event::Character('q') || e == Event::Character('Q')) {
        save_game();
        screen_.ExitLoopClosure()();
        return true;
    }

    return false;
}

// ─── Rendering ────────────────────────────────────────────────────────────────

ftxui::Element Game::render() {
    return vbox({
        render_hud(),
        hbox({
            render_map() | flex,
            separator(),
            render_panel() | size(WIDTH, EQUAL, 32),
        }) | flex,
        render_statusbar(),
    });
}

ftxui::Element Game::render_hud() {
    auto* exp = ecs_.registry().getComponent<Experience>(player_);
    auto* pl = ecs_.registry().getComponent<Player>(player_);
    int lv = exp ? exp->level : 1;
    int cur = exp ? exp->current_xp : 0;
    int nxt = exp ? exp->xp_to_next : 100;

    // XP progress bar (20 segments)
    int filled = nxt > 0 ? (cur * 20 / nxt) : 0;
    std::string bar;
    for (int i = 0; i < 20; i++)
        bar += (i < filled) ? "=" : "-";

    int gold = total_gold();
    int goldcap = total_gold_cap();
    int elix = total_elixir();
    int elixcap = total_elixir_cap();
    int builders = pl ? pl->builder_slots : 1;
    int busy = pl ? pl->busy_builders : 0;

    const Map& m = world_.current_map();

    return hbox({
               text(fmt::format(" {} ", m.name)) | bold,
               separator(),
               text(fmt::format(" Lv.{} [{}] {}/{} XP ", lv, bar, cur, nxt)) | color(Color::Yellow),
               separator(),
               text(fmt::format(" Gold: {}/{} ", gold, goldcap)) | color(Color::Gold1),
               separator(),
               text(fmt::format(" Elixir: {}/{} ", elix, elixcap)) | color(Color::MediumPurple1),
               separator(),
               text(fmt::format(" Builders: {}/{} ", builders - busy, builders)) |
                   color(Color::Cyan1),
           }) |
           border;
}

ftxui::Element Game::render_cell(int x, int y) {
    auto* pl = ecs_.registry().getComponent<Player>(player_);
    auto* pos = ecs_.registry().getComponent<Position>(player_);
    const Map& m = world_.current_map();

    bool is_player = (pos && pos->x == x && pos->y == y);
    if (is_player)
        return text("@ ") | bold | color(Color::White);

    int cell = m.grid[y][x];
    if (cell == Cell::Wall)
        return text("##") | color(Color::GrayDark);
    if (cell == Cell::Gate)
        return text(">>") | color(Color::Magenta);

    ecse::Entity e = m.get_entity(x, y);
    if (!e)
        return text("..") | color(Color::GrayDark);

    bool is_sel = (e == selected_);
    bool under_con = (ecs_.registry().getComponent<Construction>(e) != nullptr);

    if (ecs_.registry().getComponent<Obstacle>(e)) {
        auto* obs = ecs_.registry().getComponent<Obstacle>(e);
        bool clearing = obs && obs->time_remaining_s > 0.f;
        auto el = text("~~") | color(Color::Green);
        return clearing ? (el | dim) : el;
    }

    auto* bld = ecs_.registry().getComponent<Building>(e);
    if (!bld)
        return text("??") | color(Color::GrayDark);

    std::string sym(Assets::def(bld->type).symbol);
    Color col = Color::White;
    switch (bld->type) {
    case BuildingType::TownHall:
        col = Color::Yellow;
        break;
    case BuildingType::GoldMine:
        col = Color::Gold1;
        break;
    case BuildingType::ElixirCollector:
        col = Color::MediumPurple1;
        break;
    case BuildingType::GoldStorage:
        col = Color::Gold3;
        break;
    case BuildingType::ElixirStorage:
        col = Color::Purple;
        break;
    case BuildingType::BuilderHut:
        col = Color::Cyan1;
        break;
    case BuildingType::Farm:
        col = Color::Green3;
        break;
    default:
        col = Color::GrayLight;
        break;
    }

    auto el = text(under_con ? "[]" : sym) | color(col);
    if (is_sel)
        el = el | inverted;
    return el;
}

ftxui::Element Game::render_map() {
    auto* pos = ecs_.registry().getComponent<Position>(player_);
    const Map& m = world_.current_map();

    // Viewport: show up to 20 cols × 12 rows centered on player
    constexpr int VW = 20, VH = 12;
    int cx = pos ? pos->x : m.width() / 2;
    int cy = pos ? pos->y : m.height() / 2;

    int x0 = std::max(0, std::min(cx - VW / 2, m.width() - VW));
    int y0 = std::max(0, std::min(cy - VH / 2, m.height() - VH));
    int x1 = std::min(x0 + VW, m.width());
    int y1 = std::min(y0 + VH, m.height());

    std::vector<Element> rows;
    for (int y = y0; y < y1; y++) {
        std::vector<Element> cells;
        for (int x = x0; x < x1; x++)
            cells.push_back(render_cell(x, y));
        rows.push_back(hbox(std::move(cells)));
    }

    return vbox(std::move(rows)) | border;
}

ftxui::Element Game::render_panel() {
    if (mode_ == Mode::BuildSelect) {
        const auto& d = Assets::def(build_cursor_type_);
        const auto& stat = d.stats[0];
        return vbox({
                   text(" BUILD MODE ") | bold | center,
                   separator(),
                   text(fmt::format(" {}", d.name)),
                   text(fmt::format(" Cost: {}G {}E", stat.gold_cost, stat.elixir_cost)),
                   text(fmt::format(" Time: {}", fmt_time(static_cast<float>(stat.build_time_s)))),
                   separator(),
                   text(" +/- : change type"),
                   text(" Enter: place here"),
                   text(" Esc  : cancel"),
               }) |
               border;
    }

    if (!selected_) {
        return vbox({
                   text(" No selection ") | dim | center,
                   separator(),
                   text(" E - interact/collect ") | dim,
                   text(" U - upgrade          ") | dim,
                   text(" B - build            ") | dim,
                   text(" C - clear obstacle   ") | dim,
               }) |
               border;
    }

    ecse::Entity e = selected_;
    std::vector<Element> lines;

    auto* nm = ecs_.registry().getComponent<Name>(e);
    auto* lv = ecs_.registry().getComponent<Level>(e);
    auto* bld = ecs_.registry().getComponent<Building>(e);
    auto* con = ecs_.registry().getComponent<Construction>(e);
    auto* obs = ecs_.registry().getComponent<Obstacle>(e);
    auto* prd = ecs_.registry().getComponent<ResourceProducer>(e);
    auto* sto = ecs_.registry().getComponent<ResourceStorage>(e);
    auto* uc = ecs_.registry().getComponent<UpgradeCost>(e);
    auto* bst = ecs_.registry().getComponent<Boost>(e);

    if (nm)
        lines.push_back(text(fmt::format(" {}", nm->value)) | bold);
    if (lv)
        lines.push_back(text(fmt::format(" Level {}/{}", lv->level, lv->max_level)));
    if (bld)
        lines.push_back(text(fmt::format(" HP {}/{}", bld->hp, bld->max_hp)));

    if (obs) {
        lines.push_back(separator());
        lines.push_back(text(" Obstacle") | color(Color::Green));
        lines.push_back(text(fmt::format(" Clears in: {}", fmt_time(obs->clear_time_s))));
        if (obs->time_remaining_s > 0.f)
            lines.push_back(
                text(fmt::format(" Remaining: {}", fmt_time(obs->time_remaining_s))) |
                color(Color::Yellow)
            );
        lines.push_back(text(fmt::format(" Reward: {}G {}E", obs->gold_reward, obs->elixir_reward))
        );
    }

    if (prd) {
        lines.push_back(separator());
        lines.push_back(text(
            fmt::format(" Produces: {}", prd->resource == ResourceType::Gold ? "Gold" : "Elixir")
        ));
        float eff_rate = prd->rate_per_hour * (bst ? bst->multiplier : 1.f);
        lines.push_back(text(fmt::format(" Rate: {:.0f}/hr", eff_rate)));
        if (bst)
            lines.push_back(
                text(fmt::format(" Boosted ×{:.1f}", bst->multiplier)) | color(Color::Yellow)
            );
        lines.push_back(text(fmt::format(" Stored: {:.0f}/{:.0f}", prd->stored, prd->capacity)));
    }

    if (sto) {
        lines.push_back(separator());
        lines.push_back(text(
            fmt::format(" Stores: {}", sto->resource == ResourceType::Gold ? "Gold" : "Elixir")
        ));
        lines.push_back(text(fmt::format(" {}/{}", sto->current, sto->capacity)));
    }

    if (con) {
        lines.push_back(separator());
        lines.push_back(
            text(con->kind == ConstructionKind::Build ? " Building…" : " Upgrading…") |
            color(Color::Yellow)
        );
        lines.push_back(text(fmt::format(" {} remaining", fmt_time(con->time_remaining_s))));
    } else {
        if (uc) {
            lines.push_back(separator());
            lines.push_back(text(" U — Upgrade"));
            lines.push_back(text(fmt::format("   {}G {}E", uc->gold, uc->elixir)));
        }
        if (obs && obs->time_remaining_s <= 0.f) {
            lines.push_back(separator());
            lines.push_back(text(" C — Clear obstacle"));
        }
    }

    if (prd && prd->stored >= 1.f) {
        lines.push_back(separator());
        lines.push_back(text(" E — Collect") | color(Color::Green));
    }

    return vbox(std::move(lines)) | border;
}

ftxui::Element Game::render_statusbar() {
    std::vector<Element> msg_els;
    for (const auto& msg : messages_)
        msg_els.push_back(text(" " + msg));

    if (msg_els.empty())
        msg_els.push_back(text(""));

    return vbox({
               hbox({
                   text(" WASD") | bold,
                   text(":Move "),
                   text(" E") | bold,
                   text(":Interact "),
                   text(" U") | bold,
                   text(":Upgrade "),
                   text(" C") | bold,
                   text(":Clear "),
                   text(" B") | bold,
                   text(":Build "),
                   text(" Q") | bold,
                   text(":Save+Quit "),
                   text(" ^S") | bold,
                   text(":Save "),
                   text(" ^L") | bold,
                   text(":Load"),
               }),
               vbox(std::move(msg_els)) | color(Color::GrayLight),
           }) |
           border;
}
