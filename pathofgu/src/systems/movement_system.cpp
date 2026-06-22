#include "systems/movement_system.hpp"

#include "components/ai_behavior.hpp"
#include "components/health.hpp"
#include "components/position.hpp"

#include <fmt/format.h>

bool move_player(
    EntityComponentRegistry& reg, World& world, Entity player, const std::string& direction
) {
    auto* pos = reg.getComponent<Position>(player);
    Map* map = world.get_map(pos->map_id);

    int dx = 0, dy = 0;
    if (direction == "north")
        dy = -1;
    else if (direction == "south")
        dy = 1;
    else if (direction == "east")
        dx = 1;
    else if (direction == "west")
        dx = -1;
    else {
        fmt::print("Unknown direction '{}'.\n", direction);
        return false;
    }

    int nx = pos->x + dx;
    int ny = pos->y + dy;

    if (nx < 0 || nx >= 10 || ny < 0 || ny >= 10)
        return false;

    int cell = map->grid[ny][nx];

    if (cell == Cell::Wall) {
        fmt::print("A solid wall blocks your path.\n");
        return false;
    }

    if (cell == Cell::Door) {
        const Door& door = map->doors.at(Map::cell_key(nx, ny));
        world.move_entity(player, pos->map_id, door.target_map);
        pos->map_id = door.target_map;
        pos->x = door.target_x;
        pos->y = door.target_y;
        fmt::print("You pass through the door...\n");
        return true;
    }

    pos->x = nx;
    pos->y = ny;
    return true;
}
