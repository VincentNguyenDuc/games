#pragma once

class Game;

class Renderer {
    const Game& g_;

public:
    explicit Renderer(const Game& g);

    void render();
    void render_hud();
    void render_map();
    void render_cell(int x, int y, int px, int py);
    void render_panel();
    void render_statusbar();
};
