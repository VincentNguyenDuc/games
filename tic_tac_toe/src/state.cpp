#include "state.hpp"

#include <algorithm>
#include <fmt/format.h>

static constexpr std::array<std::array<int, 3>, 8> WIN_LINES = {{
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},
    {0, 3, 6},
    {1, 4, 7},
    {2, 5, 8},
    {0, 4, 8},
    {2, 4, 6},
}};

GameState initial_state() {
    Board board;
    board.fill(' ');
    return {board, 'X'};
}

std::optional<GameState> make_move(GameState state, int index) {
    if (index < 0 || index >= 9 || state.board[index] != ' ')
        return std::nullopt;
    state.board[index] = state.current_player;
    return state;
}

char check_winner(const Board& board) {
    for (const auto& line : WIN_LINES) {
        char c = board[line[0]];
        if (c != ' ' && c == board[line[1]] && c == board[line[2]])
            return c;
    }
    return ' ';
}

bool is_draw(const Board& board) {
    return check_winner(board) == ' ' &&
           std::none_of(board.begin(), board.end(), [](char c) { return c == ' '; });
}

bool is_game_over(const Board& board) { return check_winner(board) != ' ' || is_draw(board); }

char next_player(char player) { return player == 'X' ? 'O' : 'X'; }

std::string render_board(const Board& board) {
    auto cell = [&](int i) -> std::string {
        return board[i] == ' ' ? std::to_string(i + 1) : std::string(1, board[i]);
    };
    return fmt::format(
        " {} | {} | {}\n---+---+---\n {} | {} | {}\n---+---+---\n {} | {} | {}\n",
        cell(0),
        cell(1),
        cell(2),
        cell(3),
        cell(4),
        cell(5),
        cell(6),
        cell(7),
        cell(8)
    );
}
