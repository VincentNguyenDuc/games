#include <catch2/catch_test_macros.hpp>

#include "state.hpp"

TEST_CASE("initial state has empty board and X starts") {
    auto state = initial_state();
    for (char c : state.board)
        REQUIRE(c == ' ');
    REQUIRE(state.current_player == 'X');
}

TEST_CASE("make_move places piece on empty cell") {
    auto state = initial_state();
    auto next = make_move(state, 4);
    REQUIRE(next.has_value());
    REQUIRE(next->board[4] == 'X');
    REQUIRE(next->current_player == 'X');
}

TEST_CASE("make_move rejects occupied cell") {
    auto state = initial_state();
    auto next = make_move(state, 0);
    REQUIRE(!make_move(*next, 0).has_value());
}

TEST_CASE("make_move rejects out-of-bounds index") {
    auto state = initial_state();
    REQUIRE(!make_move(state, -1).has_value());
    REQUIRE(!make_move(state, 9).has_value());
}

TEST_CASE("check_winner detects row win") {
    Board board;
    board.fill(' ');
    board[0] = board[1] = board[2] = 'X';
    REQUIRE(check_winner(board) == 'X');
}

TEST_CASE("check_winner detects column win") {
    Board board;
    board.fill(' ');
    board[0] = board[3] = board[6] = 'O';
    REQUIRE(check_winner(board) == 'O');
}

TEST_CASE("check_winner detects diagonal win") {
    Board board;
    board.fill(' ');
    board[2] = board[4] = board[6] = 'X';
    REQUIRE(check_winner(board) == 'X');
}

TEST_CASE("check_winner returns space when no winner") {
    auto state = initial_state();
    REQUIRE(check_winner(state.board) == ' ');
}

TEST_CASE("is_draw detects full board with no winner") {
    // X O X
    // X X O
    // O X O
    Board board = {'X', 'O', 'X', 'X', 'X', 'O', 'O', 'X', 'O'};
    REQUIRE(is_draw(board));
    REQUIRE(is_game_over(board));
}

TEST_CASE("is_draw returns false when board has winner") {
    Board board;
    board.fill(' ');
    board[0] = board[1] = board[2] = 'X';
    REQUIRE(!is_draw(board));
}

TEST_CASE("next_player alternates correctly") {
    REQUIRE(next_player('X') == 'O');
    REQUIRE(next_player('O') == 'X');
}
