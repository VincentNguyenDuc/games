#include <fmt/format.h>
#include <iostream>

#include "state.hpp"

int main() {
    fmt::print("Tic Tac Toe\n\n");

    auto state = initial_state();

    while (!is_game_over(state.board)) {
        fmt::print(
            "{}\nPlayer {}, enter cell (1-9): ", render_board(state.board), state.current_player
        );

        int cell;
        if (!(std::cin >> cell))
            break;

        auto next = make_move(state, cell - 1);
        if (!next) {
            fmt::print("Invalid move. Try again.\n\n");
            continue;
        }
        state = *next;

        if (is_game_over(state.board)) {
            fmt::print("{}", render_board(state.board));
            char winner = check_winner(state.board);
            if (winner != ' ')
                fmt::print("Player {} wins!\n", winner);
            else
                fmt::print("It's a draw!\n");
            return 0;
        }

        state.current_player = next_player(state.current_player);
    }

    return 0;
}
