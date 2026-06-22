#include "systems/loot_system.hpp"

#include "components/aperture.hpp"
#include "components/loot.hpp"
#include "components/name.hpp"
#include "components/position.hpp"

#include <fmt/format.h>
#include <random>

static std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

void process_death(EntityComponentRegistry& reg, World& world, Entity entity) {
    auto* loot = reg.getComponent<Loot>(entity);
    auto* pos = reg.getComponent<Position>(entity);
    if (!loot || !pos)
        return;

    Room* room = world.get_room(pos->room_id);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (const auto& drop : loot->drops) {
        if (dist(rng()) <= drop.chance) {
            room->dropped_worms.push_back({drop.def});
            fmt::print("  {} dropped!\n", drop.def->name);
        }
    }
}

bool pickup_worm(EntityComponentRegistry& reg, World& world, Entity player, int drop_index) {
    auto* pos = reg.getComponent<Position>(player);
    auto* aperture = reg.getComponent<Aperture>(player);
    Room* room = world.get_room(pos->room_id);

    if (drop_index < 0 || drop_index >= (int)room->dropped_worms.size()) {
        fmt::print("No worm at that index.\n");
        return false;
    }
    if ((int)aperture->worms.size() >= aperture->capacity) {
        fmt::print("Your aperture is full ({} slots).\n", aperture->capacity);
        return false;
    }

    GuWorm worm = room->dropped_worms[drop_index];
    aperture->worms.push_back(worm);
    room->dropped_worms.erase(room->dropped_worms.begin() + drop_index);
    fmt::print("You refine {} into your aperture.\n", worm.def->name);
    return true;
}
