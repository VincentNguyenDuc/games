#pragma once

class Game;

class InputHandler {
    Game& g_;

public:
    explicit InputHandler(Game& g);
    void handle_input();
};
