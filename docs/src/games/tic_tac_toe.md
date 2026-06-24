# Tic Tac Toe

A classic two-player terminal game. X always goes first. Players alternate placing marks until one wins or the board fills.

## How to Play

The board is numbered 0–8, left-to-right, top-to-bottom:

```
 0 | 1 | 2
-----------
 3 | 4 | 5
-----------
 6 | 7 | 8
```

Enter the index of the cell you want to mark. Invalid moves (occupied cell, out of bounds) are rejected and the turn does not advance.

## Rules

- **Win** — place three marks in a row, column, or diagonal.
- **Draw** — board is full with no winner.

## Architecture

All state is modeled as pure functions over a `GameState` value type, with no mutation:

```cpp
struct GameState {
    std::array<char, 9> board;  // ' ', 'X', or 'O'
    char current_player;        // 'X' or 'O'
};
```

### Functions

| Function | Signature | Description |
|---|---|---|
| `initial_state` | `() → GameState` | Empty board, X starts |
| `make_move` | `(GameState, int) → optional<GameState>` | Returns new state or `nullopt` if move is invalid |
| `check_winner` | `(Board) → char` | Returns winning player or `' '` |
| `is_draw` | `(Board) → bool` | True if full board and no winner |
| `is_game_over` | `(Board) → bool` | True if winner or draw |
| `next_player` | `(char) → char` | Toggles between `'X'` and `'O'` |
| `render_board` | `(Board) → string` | ASCII 3×3 grid |

The purely functional design makes the game logic trivially testable — every function is deterministic and side-effect-free.
