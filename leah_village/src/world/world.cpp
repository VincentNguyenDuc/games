#include "world/world.hpp"

World::World() {
    build_home_village();   // Map 0 — always unlocked
    build_ancient_forest(); // Map 1 — unlock at level 3
    build_crystal_caves();  // Map 2 — unlock at level 7
}

// Helper: fill map perimeter with Wall cells.
static void border(Map& m) {
    for (int x = 0; x < m.width(); x++) {
        m.grid[0][x] = Cell::Wall;
        m.grid[m.height() - 1][x] = Cell::Wall;
    }
    for (int y = 0; y < m.height(); y++) {
        m.grid[y][0] = Cell::Wall;
        m.grid[y][m.width() - 1] = Cell::Wall;
    }
}

void World::build_home_village() {
    Map m(0, "Home Village", "Your starting grounds.", 22, 14);
    m.required_player_level = 1;
    border(m);

    // Gate at bottom-centre → Ancient Forest
    m.add_gate(11, 13, {1, 11, 1}); // arrive at Forest row 1 col 11

    maps_.push_back(std::move(m));
}

void World::build_ancient_forest() {
    Map m(1, "Ancient Forest", "Dense trees hide ancient elixir springs.", 20, 12);
    m.required_player_level = 3;
    // No solid border — it's an open forest; just the cells outside bounds are void.

    // Gate back to Home Village (top-centre)
    m.add_gate(10, 0, {0, 11, 12});

    // Gate to Crystal Caves (right edge)
    m.add_gate(19, 6, {2, 1, 9});

    maps_.push_back(std::move(m));
}

void World::build_crystal_caves() {
    Map m(2, "Crystal Caves", "Glittering caves rich with gold deposits.", 26, 16);
    m.required_player_level = 7;
    border(m);

    // Gate back to Ancient Forest (left edge mid)
    m.add_gate(1, 9, {1, 18, 6});

    maps_.push_back(std::move(m));
}
