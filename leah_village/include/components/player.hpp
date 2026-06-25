#pragma once

#include <cstdint>

// Singleton component — only the player entity carries this.
struct Player {
    std::uint32_t current_map_id{0};
    int builder_slots{1}; // total builder slots owned
    int busy_builders{0}; // currently building/upgrading
    bool visited_forest{false};
    bool visited_caves{false};
};
