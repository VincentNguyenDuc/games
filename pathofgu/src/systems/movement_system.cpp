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

    // Block movement if living enemies remain
    for (Entity e : map->entities) {
        if (e == player)
            continue;
        if (reg.getComponent<AIBehavior>(e) && reg.getComponent<Health>(e)) {
            fmt::print("You cannot flee while enemies are present!\n");
            return false;
        }
    }

    auto it = map->exits.find(direction);
    if (it == map->exits.end()) {
        fmt::print("There is no exit to the {}.\n", direction);
        return false;
    }

    MapId dest = it->second;
    world.move_entity(player, pos->map_id, dest);
    pos->map_id = dest;
    return true;
}
