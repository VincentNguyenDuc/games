#include "systems/movement_system.hpp"

#include "components/move_intent.hpp"
#include "components/name.hpp"
#include "components/position.hpp"

#include <fmt/format.h>

std::string resolve_moves(EntityComponentRegistry& reg, World& world) {
    std::string out;

    for (Entity e : reg.view<MoveIntentComponent>()) {
        auto* intent = reg.getComponent<MoveIntentComponent>(e);
        auto* pos = reg.getComponent<Position>(e);

        if (!intent || !pos) {
            reg.removeComponent<MoveIntentComponent>(e);
            continue;
        }

        Map* map = world.get_map(pos->map_id);

        // Returns true and applies the move if the cell at (pos+delta) is reachable.
        auto try_move = [&](int dx, int dy) -> bool {
            if (dx == 0 && dy == 0)
                return false;
            int nx = pos->x + dx;
            int ny = pos->y + dy;

            if (nx < 0 || nx >= 10 || ny < 0 || ny >= 10)
                return false;
            int cell = map->grid[ny][nx];
            if (cell == Cell::Wall)
                return false;

            if (cell == Cell::Door) {
                const Door& door = map->doors.at(Map::cell_key(nx, ny));
                world.move_entity(e, pos->map_id, door.target_map);
                pos->map_id = door.target_map;
                pos->x = door.target_x;
                pos->y = door.target_y;
                auto* n = reg.getComponent<Name>(e);
                out += fmt::format("{} passes through a door.\n", n ? n->value : "Someone");
                return true;
            }

            if (cell != Cell::Empty)
                return false;

            // Block if another entity already occupies the target cell.
            for (Entity other : map->entities) {
                if (other == e)
                    continue;
                auto* opos = reg.getComponent<Position>(other);
                if (opos && opos->x == nx && opos->y == ny)
                    return false;
            }

            pos->x = nx;
            pos->y = ny;
            return true;
        };

        bool moved = try_move(intent->dx, intent->dy);

        if (!moved && intent->allow_slide) {
            // Try horizontal then vertical fallback (mirrors diagonal-prefer pathfinding).
            if (!try_move(intent->dx, 0))
                try_move(0, intent->dy);
        }

        reg.removeComponent<MoveIntentComponent>(e);
    }

    return out;
}
