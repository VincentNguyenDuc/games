#include "input.hpp"

#include "actions.hpp"
#include "game.hpp"

#include <raylib.h>

InputHandler::InputHandler(Game& g)
    : g_(g) {}

// ─── Input ────────────────────────────────────────────────────────────────────

void InputHandler::handle_input() {
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

    Actions& actions = *g_.actions_;

    if (g_.mode_ == Game::Mode::BuildSelect) {
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            if (ch == '+') {
                g_.build_menu_idx_ = (g_.build_menu_idx_ + 1) % N;
                g_.build_cursor_type_ = BUILD_TYPES[g_.build_menu_idx_];
            } else if (ch == '-') {
                g_.build_menu_idx_ = (g_.build_menu_idx_ + N - 1) % N;
                g_.build_cursor_type_ = BUILD_TYPES[g_.build_menu_idx_];
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            g_.build_menu_idx_ = (g_.build_menu_idx_ + 1) % N;
            g_.build_cursor_type_ = BUILD_TYPES[g_.build_menu_idx_];
        }
        if (IsKeyPressed(KEY_LEFT)) {
            g_.build_menu_idx_ = (g_.build_menu_idx_ + N - 1) % N;
            g_.build_cursor_type_ = BUILD_TYPES[g_.build_menu_idx_];
        }
        if (IsKeyPressed(KEY_ENTER)) {
            actions.confirm_build();
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            actions.cancel_build();
            return;
        }

        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
            actions.try_move(0, -1);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
            actions.try_move(0, 1);
        if (IsKeyPressed(KEY_A))
            actions.try_move(-1, 0);
        if (IsKeyPressed(KEY_D))
            actions.try_move(1, 0);
        return;
    }

    // Normal mode
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
        actions.try_move(0, -1);
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
        actions.try_move(0, 1);
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
        actions.try_move(-1, 0);
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
        actions.try_move(1, 0);

    if (IsKeyPressed(KEY_E))
        actions.try_interact();
    if (IsKeyPressed(KEY_U))
        actions.try_upgrade();
    if (IsKeyPressed(KEY_C))
        actions.try_clear();
    if (IsKeyPressed(KEY_B))
        actions.enter_build_mode();

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrl && IsKeyPressed(KEY_S)) {
        g_.save_game();
        return;
    }
    if (ctrl && IsKeyPressed(KEY_L)) {
        g_.load_game();
        return;
    }

    if (IsKeyPressed(KEY_Q)) {
        g_.save_game();
        g_.quit_ = true;
    }
}
