#include "game.hpp"

#include "assets.hpp"
#include "persistence/save_load.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <raylib.h>
#include <spdlog/spdlog.h>

// ─── Construction ────────────────────────────────────────────────────────────

Game::Game() {
    setup_ecs();
    spawn_initial_buildings();

    last_tick_ = std::chrono::steady_clock::now();

    if (std::filesystem::exists(Game::DB_PATH))
        load_game();
}

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

    spawn_obstacle(0, 7, 2, 25, 0, 5.f);
    spawn_obstacle(0, 13, 2, 0, 25, 5.f);
    spawn_obstacle(0, 7, 8, 30, 0, 5.f);
    spawn_obstacle(0, 13, 8, 0, 30, 5.f);

    // ── Ancient Forest (Map 1) ───────────────────────────────────────────────
    spawn_building(1, 5, 5, BuildingType::ElixirCollector);
    spawn_building(1, 13, 5, BuildingType::ElixirCollector);
    spawn_building(1, 9, 8, BuildingType::Farm);

    for (auto [ox, oy] : std::initializer_list<std::pair<int, int>>{
             {2, 2}, {3, 7}, {7, 3}, {11, 8}, {15, 2}, {15, 7}, {6, 9}, {12, 3}})
        spawn_obstacle(1, ox, oy, 0, 40, 10.f);

    // ── Crystal Caves (Map 2) ────────────────────────────────────────────────
    spawn_building(2, 6, 4, BuildingType::GoldMine, 2);
    spawn_building(2, 18, 4, BuildingType::GoldMine, 2);
    spawn_building(2, 12, 8, BuildingType::GoldStorage, 2);
    spawn_building(2, 4, 11, BuildingType::Farm);

    for (auto [ox, oy] : std::initializer_list<std::pair<int, int>>{
             {9, 3}, {14, 3}, {3, 6}, {21, 6}, {9, 12}, {14, 12}})
        spawn_obstacle(2, ox, oy, 100, 0, 15.f);
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

        handle_input();
        tick(dt);

        BeginDrawing();
        ClearBackground({15, 15, 25, 255});
        render();
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

    if (auto* prod = ecs_.registry().getComponent<ResourceProducer>(e)) {
        if (prod->stored >= 1.f) {
            do_collect(e);
            return;
        }
    }

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
    push_msg("Build mode: +/- to pick type, Enter to place, Esc to cancel.");
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

// ─── Input ────────────────────────────────────────────────────────────────────

void Game::handle_input() {
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

    if (mode_ == Mode::BuildSelect) {
        // Cycle building type with +/- (character based) or Left/Right arrows
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            if (ch == '+') {
                build_menu_idx_ = (build_menu_idx_ + 1) % N;
                build_cursor_type_ = BUILD_TYPES[build_menu_idx_];
            } else if (ch == '-') {
                build_menu_idx_ = (build_menu_idx_ + N - 1) % N;
                build_cursor_type_ = BUILD_TYPES[build_menu_idx_];
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            build_menu_idx_ = (build_menu_idx_ + 1) % N;
            build_cursor_type_ = BUILD_TYPES[build_menu_idx_];
        }
        if (IsKeyPressed(KEY_LEFT)) {
            build_menu_idx_ = (build_menu_idx_ + N - 1) % N;
            build_cursor_type_ = BUILD_TYPES[build_menu_idx_];
        }
        if (IsKeyPressed(KEY_ENTER)) {
            confirm_build();
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            cancel_build();
            return;
        }

        // Allow WASD movement in build mode to reposition player
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
            try_move(0, -1);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
            try_move(0, 1);
        if (IsKeyPressed(KEY_A))
            try_move(-1, 0);
        if (IsKeyPressed(KEY_D))
            try_move(1, 0);
        return;
    }

    // Normal mode movement
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
        try_move(0, -1);
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
        try_move(0, 1);
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
        try_move(-1, 0);
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
        try_move(1, 0);

    if (IsKeyPressed(KEY_E))
        try_interact();
    if (IsKeyPressed(KEY_U))
        try_upgrade();
    if (IsKeyPressed(KEY_C))
        try_clear();
    if (IsKeyPressed(KEY_B))
        enter_build_mode();

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrl && IsKeyPressed(KEY_S)) {
        save_game();
        return;
    }
    if (ctrl && IsKeyPressed(KEY_L)) {
        load_game();
        return;
    }

    if (IsKeyPressed(KEY_Q)) {
        save_game();
        quit_ = true;
    }
}

// ─── Rendering ────────────────────────────────────────────────────────────────

void Game::render() {
    render_hud();
    render_map();
    render_panel();
    render_statusbar();
}

// Panel divider line
static void panel_divider(int panel_x, int window_w, int y) {
    DrawLine(panel_x + 8, y, window_w - 8, y, {70, 70, 100, 255});
}

void Game::render_hud() {
    DrawRectangle(0, 0, WINDOW_W, HUD_H, {28, 28, 48, 255});
    DrawLine(0, HUD_H - 1, WINDOW_W, HUD_H - 1, {70, 70, 110, 255});

    auto* exp = ecs_.registry().getComponent<Experience>(player_);
    auto* pl = ecs_.registry().getComponent<Player>(player_);
    int lv = exp ? exp->level : 1;
    int cur = exp ? exp->current_xp : 0;
    int nxt = exp ? exp->xp_to_next : 100;
    int gold = total_gold(), goldcap = total_gold_cap();
    int elix = total_elixir(), elixcap = total_elixir_cap();
    int builders = pl ? pl->builder_slots : 1;
    int busy = pl ? pl->busy_builders : 0;
    const Map& m = world_.current_map();

    constexpr int FS = 20;
    constexpr int TY = 18;
    int x = 12;

    // Map name
    DrawText(m.name.c_str(), x, TY, FS, WHITE);
    x += MeasureText(m.name.c_str(), FS) + 18;

    // Separator
    DrawLine(x, 8, x, HUD_H - 8, {70, 70, 110, 255});
    x += 12;

    // Level + XP bar
    auto lv_str = fmt::format("Lv.{}", lv);
    DrawText(lv_str.c_str(), x, TY, FS, YELLOW);
    x += MeasureText(lv_str.c_str(), FS) + 8;

    constexpr int BAR_W = 120, BAR_H = 14;
    int filled = (nxt > 0) ? (cur * BAR_W / nxt) : 0;
    DrawRectangle(x, TY + 3, BAR_W, BAR_H, {60, 60, 0, 255});
    DrawRectangle(x, TY + 3, filled, BAR_H, {220, 200, 0, 255});
    DrawRectangleLines(x, TY + 3, BAR_W, BAR_H, {120, 120, 0, 255});
    x += BAR_W + 6;

    auto xp_str = fmt::format("{}/{}", cur, nxt);
    DrawText(xp_str.c_str(), x, TY, FS - 2, {180, 180, 0, 255});
    x += MeasureText(xp_str.c_str(), FS - 2) + 18;

    DrawLine(x, 8, x, HUD_H - 8, {70, 70, 110, 255});
    x += 12;

    // Gold
    auto gold_str = fmt::format("Gold: {}/{}", gold, goldcap);
    DrawText(gold_str.c_str(), x, TY, FS, {255, 200, 0, 255});
    x += MeasureText(gold_str.c_str(), FS) + 18;

    DrawLine(x, 8, x, HUD_H - 8, {70, 70, 110, 255});
    x += 12;

    // Elixir
    auto elix_str = fmt::format("Elixir: {}/{}", elix, elixcap);
    DrawText(elix_str.c_str(), x, TY, FS, {180, 100, 255, 255});
    x += MeasureText(elix_str.c_str(), FS) + 18;

    DrawLine(x, 8, x, HUD_H - 8, {70, 70, 110, 255});
    x += 12;

    // Builders
    auto bld_str = fmt::format("Builders: {}/{}", builders - busy, builders);
    DrawText(bld_str.c_str(), x, TY, FS, {80, 200, 255, 255});
}

void Game::render_map() {
    auto* pos = ecs_.registry().getComponent<Position>(player_);
    const Map& m = world_.current_map();

    constexpr int VW = VIEWPORT_COLS, VH = VIEWPORT_ROWS;
    int cx = pos ? pos->x : m.width() / 2;
    int cy = pos ? pos->y : m.height() / 2;

    int x0 = std::max(0, std::min(cx - VW / 2, m.width() - VW));
    int y0 = std::max(0, std::min(cy - VH / 2, m.height() - VH));
    int x1 = std::min(x0 + VW, m.width());
    int y1 = std::min(y0 + VH, m.height());

    DrawRectangle(0, MAP_Y, MAP_PX_W, MAP_PX_H, {18, 18, 30, 255});

    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            render_cell(x, y, (x - x0) * TILE_W, MAP_Y + (y - y0) * TILE_H);

    DrawRectangleLines(0, MAP_Y, MAP_PX_W, MAP_PX_H, {70, 70, 110, 255});
}

void Game::render_cell(int x, int y, int px, int py) {
    auto* pl_pos = ecs_.registry().getComponent<Position>(player_);
    const Map& m = world_.current_map();

    bool is_player = (pl_pos && pl_pos->x == x && pl_pos->y == y);
    int cell = m.grid[y][x];
    ecse::Entity e = m.get_entity(x, y);
    bool is_sel = (e && e == selected_);

    // ── Background ────────────────────────────────────────────────────────────
    Color bg = {18, 18, 30, 255};
    if (cell == Cell::Wall)
        bg = {55, 55, 65, 255};
    else if (cell == Cell::Gate)
        bg = {70, 0, 90, 255};
    else if (is_sel)
        bg = {50, 50, 75, 255};

    DrawRectangle(px, py, TILE_W - 1, TILE_H - 1, bg);

    // ── Symbol + foreground color ─────────────────────────────────────────────
    const char* sym = nullptr;
    Color col = GRAY;

    if (is_player) {
        sym = "@";
        col = WHITE;
        DrawRectangle(px, py, TILE_W - 1, TILE_H - 1, {40, 40, 60, 255});
    } else if (cell == Cell::Wall) {
        sym = "#";
        col = {90, 90, 100, 255};
    } else if (cell == Cell::Gate) {
        sym = ">>";
        col = {220, 120, 255, 255};
    } else if (e) {
        if (auto* obs = ecs_.registry().getComponent<Obstacle>(e)) {
            bool clearing = obs->time_remaining_s > 0.f;
            sym = clearing ? "~~" : "~~";
            col = clearing ? Color{0, 140, 0, 255} : Color{0, 210, 60, 255};
            DrawRectangle(px, py, TILE_W - 1, TILE_H - 1, {0, 40, 0, 255});
        } else if (auto* bld = ecs_.registry().getComponent<Building>(e)) {
            bool under_con = (ecs_.registry().getComponent<Construction>(e) != nullptr);
            auto sym_sv = under_con ? std::string_view("[]") : Assets::def(bld->type).symbol;
            // borrow a static buffer so DrawText gets a null-terminated string
            static char sym_buf[8];
            sym_buf[0] = sym_sv[0];
            sym_buf[1] = sym_sv.size() > 1 ? sym_sv[1] : '\0';
            sym_buf[2] = '\0';
            sym = sym_buf;

            Color tile_bg = bg;
            switch (bld->type) {
            case BuildingType::TownHall:
                col = YELLOW;
                tile_bg = {70, 70, 0, 255};
                break;
            case BuildingType::GoldMine:
                col = {255, 200, 0, 255};
                tile_bg = {65, 55, 0, 255};
                break;
            case BuildingType::ElixirCollector:
                col = {200, 120, 255, 255};
                tile_bg = {50, 0, 80, 255};
                break;
            case BuildingType::GoldStorage:
                col = {220, 170, 0, 255};
                tile_bg = {55, 45, 0, 255};
                break;
            case BuildingType::ElixirStorage:
                col = {160, 80, 240, 255};
                tile_bg = {40, 0, 65, 255};
                break;
            case BuildingType::BuilderHut:
                col = {80, 200, 255, 255};
                tile_bg = {0, 50, 80, 255};
                break;
            case BuildingType::Farm:
                col = {80, 220, 80, 255};
                tile_bg = {0, 55, 0, 255};
                break;
            default:
                col = LIGHTGRAY;
                tile_bg = {40, 40, 55, 255};
                break;
            }
            if (under_con) {
                col = {200, 200, 60, 255};
                tile_bg = {50, 50, 0, 255};
            }
            if (is_sel)
                tile_bg = {
                    static_cast<unsigned char>(tile_bg.r + 20),
                    static_cast<unsigned char>(tile_bg.g + 20),
                    static_cast<unsigned char>(tile_bg.b + 20),
                    255};
            DrawRectangle(px, py, TILE_W - 1, TILE_H - 1, tile_bg);
        }
    } else {
        sym = ".";
        col = {45, 45, 65, 255};
    }

    // ── Draw symbol centered in tile ─────────────────────────────────────────
    if (sym) {
        constexpr int FS = 18;
        int tw = MeasureText(sym, FS);
        DrawText(sym, px + (TILE_W - 1 - tw) / 2, py + (TILE_H - 1 - FS) / 2, FS, col);
    }

    // ── Selection border ─────────────────────────────────────────────────────
    if (is_sel)
        DrawRectangleLines(px, py, TILE_W - 1, TILE_H - 1, WHITE);
}

void Game::render_panel() {
    DrawRectangle(PANEL_X, 0, PANEL_W, WINDOW_H, {22, 22, 38, 255});
    DrawLine(PANEL_X, 0, PANEL_X, WINDOW_H, {70, 70, 110, 255});

    constexpr int X = PANEL_X + 14;
    constexpr int FS = 18;
    constexpr int LH = 24;
    int y = 12;

    if (mode_ == Mode::BuildSelect) {
        DrawText("BUILD MODE", X, y, FS + 3, YELLOW);
        y += LH + 6;
        panel_divider(PANEL_X, WINDOW_W, y);
        y += 10;

        const auto& d = Assets::def(build_cursor_type_);
        const auto& stat = d.stats[0];
        std::string name_s(d.name);

        DrawText(name_s.c_str(), X, y, FS + 2, WHITE);
        y += LH + 4;
        DrawText(
            fmt::format("Cost: {}G  {}E", stat.gold_cost, stat.elixir_cost).c_str(),
            X,
            y,
            FS,
            {200, 200, 200, 255}
        );
        y += LH;
        DrawText(
            fmt::format("Time: {}", fmt_time(static_cast<float>(stat.build_time_s))).c_str(),
            X,
            y,
            FS,
            {200, 200, 200, 255}
        );
        y += LH + 8;

        panel_divider(PANEL_X, WINDOW_W, y);
        y += 10;
        DrawText("+/- or </> : change type", X, y, FS - 2, {160, 160, 180, 255});
        y += LH;
        DrawText("Enter : place here", X, y, FS - 2, {160, 160, 180, 255});
        y += LH;
        DrawText("Esc   : cancel", X, y, FS - 2, {160, 160, 180, 255});
        y += LH;

        return;
    }

    if (!selected_) {
        DrawText("No selection", X, y, FS, {120, 120, 140, 255});
        y += LH + 8;
        panel_divider(PANEL_X, WINDOW_W, y);
        y += 10;
        const Color hint = {100, 100, 120, 255};
        DrawText("WASD / arrows : move", X, y, FS - 2, hint);
        y += LH;
        DrawText("E : interact / collect", X, y, FS - 2, hint);
        y += LH;
        DrawText("U : upgrade", X, y, FS - 2, hint);
        y += LH;
        DrawText("B : build", X, y, FS - 2, hint);
        y += LH;
        DrawText("C : clear obstacle", X, y, FS - 2, hint);
        y += LH;
        DrawText("Q : save + quit", X, y, FS - 2, hint);
        y += LH;
        DrawText("Ctrl+S : save", X, y, FS - 2, hint);
        y += LH;
        DrawText("Ctrl+L : load", X, y, FS - 2, hint);
        y += LH;
        return;
    }

    ecse::Entity e = selected_;
    auto* nm = ecs_.registry().getComponent<Name>(e);
    auto* lv = ecs_.registry().getComponent<Level>(e);
    auto* bld = ecs_.registry().getComponent<Building>(e);
    auto* con = ecs_.registry().getComponent<Construction>(e);
    auto* obs = ecs_.registry().getComponent<Obstacle>(e);
    auto* prd = ecs_.registry().getComponent<ResourceProducer>(e);
    auto* sto = ecs_.registry().getComponent<ResourceStorage>(e);
    auto* uc = ecs_.registry().getComponent<UpgradeCost>(e);
    auto* bst = ecs_.registry().getComponent<Boost>(e);

    if (nm) {
        DrawText(nm->value.c_str(), X, y, FS + 4, WHITE);
        y += LH + 6;
    }
    if (lv) {
        DrawText(
            fmt::format("Level {}/{}", lv->level, lv->max_level).c_str(),
            X,
            y,
            FS,
            {180, 180, 200, 255}
        );
        y += LH;
    }
    if (bld) {
        DrawText(
            fmt::format("HP {}/{}", bld->hp, bld->max_hp).c_str(), X, y, FS, {180, 180, 200, 255}
        );
        y += LH;
    }

    if (obs) {
        y += 4;
        panel_divider(PANEL_X, WINDOW_W, y);
        y += 10;
        DrawText("Obstacle", X, y, FS, {80, 210, 80, 255});
        y += LH;
        DrawText(
            fmt::format("Clears in: {}", fmt_time(obs->clear_time_s)).c_str(),
            X,
            y,
            FS - 2,
            {160, 160, 180, 255}
        );
        y += LH;
        if (obs->time_remaining_s > 0.f) {
            DrawText(
                fmt::format("Remaining: {}", fmt_time(obs->time_remaining_s)).c_str(),
                X,
                y,
                FS - 2,
                YELLOW
            );
            y += LH;
        }
        DrawText(
            fmt::format("Reward: {}G  {}E", obs->gold_reward, obs->elixir_reward).c_str(),
            X,
            y,
            FS - 2,
            {200, 190, 100, 255}
        );
        y += LH;
    }

    if (prd) {
        y += 4;
        panel_divider(PANEL_X, WINDOW_W, y);
        y += 10;
        const char* rname = prd->resource == ResourceType::Gold ? "Gold" : "Elixir";
        Color rcol = prd->resource == ResourceType::Gold ? Color{255, 200, 0, 255}
                                                         : Color{180, 100, 255, 255};
        DrawText(fmt::format("Produces: {}", rname).c_str(), X, y, FS, rcol);
        y += LH;
        float eff = prd->rate_per_hour * (bst ? bst->multiplier : 1.f);
        DrawText(fmt::format("Rate: {:.0f}/hr", eff).c_str(), X, y, FS - 2, {160, 160, 180, 255});
        y += LH;
        if (bst) {
            DrawText(fmt::format("Boosted x{:.1f}", bst->multiplier).c_str(), X, y, FS - 2, YELLOW);
            y += LH;
        }
        DrawText(
            fmt::format("Stored: {:.0f}/{:.0f}", prd->stored, prd->capacity).c_str(),
            X,
            y,
            FS - 2,
            {160, 160, 180, 255}
        );
        y += LH;
    }

    if (sto) {
        y += 4;
        panel_divider(PANEL_X, WINDOW_W, y);
        y += 10;
        const char* rname = sto->resource == ResourceType::Gold ? "Gold" : "Elixir";
        Color rcol = sto->resource == ResourceType::Gold ? Color{255, 200, 0, 255}
                                                         : Color{180, 100, 255, 255};
        DrawText(fmt::format("Stores: {}", rname).c_str(), X, y, FS, rcol);
        y += LH;
        DrawText(
            fmt::format("{}/{}", sto->current, sto->capacity).c_str(),
            X,
            y,
            FS - 2,
            {160, 160, 180, 255}
        );
        y += LH;
    }

    if (con) {
        y += 4;
        panel_divider(PANEL_X, WINDOW_W, y);
        y += 10;
        const char* clbl = con->kind == ConstructionKind::Build ? "Building..." : "Upgrading...";
        DrawText(clbl, X, y, FS, YELLOW);
        y += LH;
        DrawText(
            fmt::format("{} remaining", fmt_time(con->time_remaining_s)).c_str(),
            X,
            y,
            FS - 2,
            {160, 160, 180, 255}
        );
        y += LH;
    } else {
        if (uc) {
            y += 4;
            panel_divider(PANEL_X, WINDOW_W, y);
            y += 10;
            DrawText("U  —  Upgrade", X, y, FS, {100, 200, 255, 255});
            y += LH;
            DrawText(
                fmt::format("  {}G  {}E", uc->gold, uc->elixir).c_str(),
                X,
                y,
                FS - 2,
                {160, 160, 180, 255}
            );
            y += LH;
        }
        if (obs && obs->time_remaining_s <= 0.f) {
            y += 4;
            panel_divider(PANEL_X, WINDOW_W, y);
            y += 10;
            DrawText("C  —  Clear obstacle", X, y, FS, {80, 210, 80, 255});
            y += LH;
        }
    }

    if (prd && prd->stored >= 1.f) {
        y += 4;
        panel_divider(PANEL_X, WINDOW_W, y);
        y += 10;
        DrawText("E  —  Collect", X, y, FS, {80, 210, 80, 255});
        y += LH;
    }
}

void Game::render_statusbar() {
    DrawRectangle(0, STATUS_Y, MAP_PX_W, STATUS_H, {20, 20, 34, 255});
    DrawLine(0, STATUS_Y, MAP_PX_W, STATUS_Y, {70, 70, 110, 255});

    constexpr int X = 10;
    constexpr int FS = 15;
    constexpr int LH = 20;
    int y = STATUS_Y + 8;

    DrawText(
        "WASD:Move  E:Interact  U:Upgrade  C:Clear  B:Build  Q:Quit  Ctrl+S:Save  Ctrl+L:Load",
        X,
        y,
        FS,
        {80, 80, 100, 255}
    );
    y += LH + 4;

    for (const auto& msg : messages_) {
        DrawText(msg.c_str(), X, y, FS + 1, {190, 190, 210, 255});
        y += LH;
        if (y > WINDOW_H - LH)
            break;
    }
}
