#pragma once

#include <array>
#include <optional>
#include <string>

using Board = std::array<char, 9>;

struct GameState {
    Board board;
    char current_player;
};

GameState initial_state();
std::optional<GameState> make_move(GameState state, int index);
char check_winner(const Board& board);
bool is_draw(const Board& board);
bool is_game_over(const Board& board);
char next_player(char player);
std::string render_board(const Board& board);
