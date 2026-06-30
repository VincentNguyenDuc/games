#include "assets.hpp"
#include "game.hpp"
#include "renderer.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <raylib.h>

Renderer::Renderer(const Game& g)
    : g_(g) {}

// ─── Helpers ─────────────────────────────────────────────────────────────────

static void panel_divider(int panel_x, int window_w, int y) {
    DrawLine(panel_x + 8, y, window_w - 8, y, {70, 70, 100, 255});
}

// ─── Top-level ────────────────────────────────────────────────────────────────

void Renderer::render() {
    render_hud();
    render_map();
    render_panel();
    render_statusbar();
}

// ─── HUD ─────────────────────────────────────────────────────────────────────

void Renderer::render_hud() {
    DrawRectangle(0, 0, Game::WINDOW_W, Game::HUD_H, {28, 28, 48, 255});
    DrawLine(0, Game::HUD_H - 1, Game::WINDOW_W, Game::HUD_H - 1, {70, 70, 110, 255});

    auto* exp = g_.ecs_.registry().getComponent<Experience>(g_.player_);
    auto* pl = g_.ecs_.registry().getComponent<Player>(g_.player_);
    int lv = exp ? exp->level : 1;
    int cur = exp ? exp->current_xp : 0;
    int nxt = exp ? exp->xp_to_next : 100;
    int gold = g_.total_gold(), goldcap = g_.total_gold_cap();
    int elix = g_.total_elixir(), elixcap = g_.total_elixir_cap();
    int builders = pl ? pl->builder_slots : 1;
    int busy = pl ? pl->busy_builders : 0;
    const Map& m = g_.world_.current_map();

    constexpr int FS = 20;
    constexpr int TY = 18;
    int x = 12;

    DrawText(m.name.c_str(), x, TY, FS, WHITE);
    x += MeasureText(m.name.c_str(), FS) + 18;

    DrawLine(x, 8, x, Game::HUD_H - 8, {70, 70, 110, 255});
    x += 12;

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

    DrawLine(x, 8, x, Game::HUD_H - 8, {70, 70, 110, 255});
    x += 12;

    auto gold_str = fmt::format("Gold: {}/{}", gold, goldcap);
    DrawText(gold_str.c_str(), x, TY, FS, {255, 200, 0, 255});
    x += MeasureText(gold_str.c_str(), FS) + 18;

    DrawLine(x, 8, x, Game::HUD_H - 8, {70, 70, 110, 255});
    x += 12;

    auto elix_str = fmt::format("Elixir: {}/{}", elix, elixcap);
    DrawText(elix_str.c_str(), x, TY, FS, {180, 100, 255, 255});
    x += MeasureText(elix_str.c_str(), FS) + 18;

    DrawLine(x, 8, x, Game::HUD_H - 8, {70, 70, 110, 255});
    x += 12;

    auto bld_str = fmt::format("Builders: {}/{}", builders - busy, builders);
    DrawText(bld_str.c_str(), x, TY, FS, {80, 200, 255, 255});
}

// ─── Map ─────────────────────────────────────────────────────────────────────

void Renderer::render_map() {
    auto* pos = g_.ecs_.registry().getComponent<Position>(g_.player_);
    const Map& m = g_.world_.current_map();

    constexpr int VW = Game::VIEWPORT_COLS, VH = Game::VIEWPORT_ROWS;
    int cx = pos ? pos->x : m.width() / 2;
    int cy = pos ? pos->y : m.height() / 2;

    int x0 = std::max(0, std::min(cx - VW / 2, m.width() - VW));
    int y0 = std::max(0, std::min(cy - VH / 2, m.height() - VH));
    int x1 = std::min(x0 + VW, m.width());
    int y1 = std::min(y0 + VH, m.height());

    DrawRectangle(0, Game::MAP_Y, Game::MAP_PX_W, Game::MAP_PX_H, {18, 18, 30, 255});

    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            render_cell(x, y, (x - x0) * Game::TILE_W, Game::MAP_Y + (y - y0) * Game::TILE_H);

    DrawRectangleLines(0, Game::MAP_Y, Game::MAP_PX_W, Game::MAP_PX_H, {70, 70, 110, 255});
}

void Renderer::render_cell(int x, int y, int px, int py) {
    auto* pl_pos = g_.ecs_.registry().getComponent<Position>(g_.player_);
    const Map& m = g_.world_.current_map();

    bool is_player = (pl_pos && pl_pos->x == x && pl_pos->y == y);
    int cell = m.grid[y][x];
    ecse::Entity e = m.get_entity(x, y);
    bool is_sel = (e && e == g_.selected_);

    Color bg = {18, 18, 30, 255};
    if (cell == Cell::Wall)
        bg = {55, 55, 65, 255};
    else if (cell == Cell::Gate)
        bg = {70, 0, 90, 255};
    else if (is_sel)
        bg = {50, 50, 75, 255};

    DrawRectangle(px, py, Game::TILE_W - 1, Game::TILE_H - 1, bg);

    const char* sym = nullptr;
    Color col = GRAY;

    if (is_player) {
        sym = "@";
        col = WHITE;
        DrawRectangle(px, py, Game::TILE_W - 1, Game::TILE_H - 1, {40, 40, 60, 255});
    } else if (cell == Cell::Wall) {
        sym = "#";
        col = {90, 90, 100, 255};
    } else if (cell == Cell::Gate) {
        sym = ">>";
        col = {220, 120, 255, 255};
    } else if (e) {
        if (auto* obs = g_.ecs_.registry().getComponent<Obstacle>(e)) {
            bool clearing = obs->time_remaining_s > 0.f;
            sym = "~~";
            col = clearing ? Color{0, 140, 0, 255} : Color{0, 210, 60, 255};
            DrawRectangle(px, py, Game::TILE_W - 1, Game::TILE_H - 1, {0, 40, 0, 255});
        } else if (auto* bld = g_.ecs_.registry().getComponent<Building>(e)) {
            bool under_con = (g_.ecs_.registry().getComponent<Construction>(e) != nullptr);
            auto sym_sv = under_con ? std::string_view("[]") : Assets::def(bld->type).symbol;
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
            DrawRectangle(px, py, Game::TILE_W - 1, Game::TILE_H - 1, tile_bg);
        }
    } else {
        sym = ".";
        col = {45, 45, 65, 255};
    }

    if (sym) {
        constexpr int FS = 18;
        int tw = MeasureText(sym, FS);
        DrawText(sym, px + (Game::TILE_W - 1 - tw) / 2, py + (Game::TILE_H - 1 - FS) / 2, FS, col);
    }

    if (is_sel)
        DrawRectangleLines(px, py, Game::TILE_W - 1, Game::TILE_H - 1, WHITE);
}

// ─── Panel ───────────────────────────────────────────────────────────────────

void Renderer::render_panel() {
    DrawRectangle(Game::PANEL_X, 0, Game::PANEL_W, Game::WINDOW_H, {22, 22, 38, 255});
    DrawLine(Game::PANEL_X, 0, Game::PANEL_X, Game::WINDOW_H, {70, 70, 110, 255});

    constexpr int X = Game::PANEL_X + 14;
    constexpr int FS = 18;
    constexpr int LH = 24;
    int y = 12;

    if (g_.mode_ == Game::Mode::BuildSelect) {
        DrawText("BUILD MODE", X, y, FS + 3, YELLOW);
        y += LH + 6;
        panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
        y += 10;

        const auto& d = Assets::def(g_.build_cursor_type_);
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
            fmt::format("Time: {}", Game::fmt_time(static_cast<float>(stat.build_time_s))).c_str(),
            X,
            y,
            FS,
            {200, 200, 200, 255}
        );
        y += LH + 8;

        panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
        y += 10;
        DrawText("+/- or </> : change type", X, y, FS - 2, {160, 160, 180, 255});
        y += LH;
        DrawText("Enter : place here", X, y, FS - 2, {160, 160, 180, 255});
        y += LH;
        DrawText("Esc   : cancel", X, y, FS - 2, {160, 160, 180, 255});
        return;
    }

    if (!g_.selected_) {
        DrawText("No selection", X, y, FS, {120, 120, 140, 255});
        y += LH + 8;
        panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
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
        return;
    }

    ecse::Entity e = g_.selected_;
    auto* nm = g_.ecs_.registry().getComponent<Name>(e);
    auto* lv = g_.ecs_.registry().getComponent<Level>(e);
    auto* bld = g_.ecs_.registry().getComponent<Building>(e);
    auto* con = g_.ecs_.registry().getComponent<Construction>(e);
    auto* obs = g_.ecs_.registry().getComponent<Obstacle>(e);
    auto* prd = g_.ecs_.registry().getComponent<ResourceProducer>(e);
    auto* sto = g_.ecs_.registry().getComponent<ResourceStorage>(e);
    auto* uc = g_.ecs_.registry().getComponent<UpgradeCost>(e);
    auto* bst = g_.ecs_.registry().getComponent<Boost>(e);

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
        panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
        y += 10;
        DrawText("Obstacle", X, y, FS, {80, 210, 80, 255});
        y += LH;
        DrawText(
            fmt::format("Clears in: {}", Game::fmt_time(obs->clear_time_s)).c_str(),
            X,
            y,
            FS - 2,
            {160, 160, 180, 255}
        );
        y += LH;
        if (obs->time_remaining_s > 0.f) {
            DrawText(
                fmt::format("Remaining: {}", Game::fmt_time(obs->time_remaining_s)).c_str(),
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
        panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
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
        panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
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
        panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
        y += 10;
        const char* clbl = con->kind == ConstructionKind::Build ? "Building..." : "Upgrading...";
        DrawText(clbl, X, y, FS, YELLOW);
        y += LH;
        DrawText(
            fmt::format("{} remaining", Game::fmt_time(con->time_remaining_s)).c_str(),
            X,
            y,
            FS - 2,
            {160, 160, 180, 255}
        );
        y += LH;
    } else {
        if (uc) {
            y += 4;
            panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
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
            panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
            y += 10;
            DrawText("C  —  Clear obstacle", X, y, FS, {80, 210, 80, 255});
            y += LH;
        }
    }

    if (prd && prd->stored >= 1.f) {
        y += 4;
        panel_divider(Game::PANEL_X, Game::WINDOW_W, y);
        y += 10;
        DrawText("E  —  Collect", X, y, FS, {80, 210, 80, 255});
    }
}

// ─── Status bar ──────────────────────────────────────────────────────────────

void Renderer::render_statusbar() {
    DrawRectangle(0, Game::STATUS_Y, Game::MAP_PX_W, Game::STATUS_H, {20, 20, 34, 255});
    DrawLine(0, Game::STATUS_Y, Game::MAP_PX_W, Game::STATUS_Y, {70, 70, 110, 255});

    constexpr int X = 10;
    constexpr int FS = 15;
    constexpr int LH = 20;
    int y = Game::STATUS_Y + 8;

    DrawText(
        "WASD:Move  E:Interact  U:Upgrade  C:Clear  B:Build  Q:Quit  Ctrl+S:Save  Ctrl+L:Load",
        X,
        y,
        FS,
        {80, 80, 100, 255}
    );
    y += LH + 4;

    for (const auto& msg : g_.messages_) {
        DrawText(msg.c_str(), X, y, FS + 1, {190, 190, 210, 255});
        y += LH;
        if (y > Game::WINDOW_H - LH)
            break;
    }
}
