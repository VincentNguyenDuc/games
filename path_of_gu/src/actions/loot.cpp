#include "actions/loot.hpp"

#include "components/aperture.hpp"
#include "components/loot.hpp"
#include "components/name.hpp"
#include "components/position.hpp"

#include <fmt/format.h>
#include <random>

using namespace ecse;

static std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

void process_death(EntityComponentRegistry& reg, World& world, Entity entity) {
    auto* loot = reg.getComponent<Loot>(entity);
    auto* pos = reg.getComponent<Position>(entity);
    if (!loot || !pos)
        return;

    Map* map = world.get_map(pos->map_id);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (const auto& drop : loot->drops) {
        if (dist(rng()) <= drop.chance) {
            map->dropped_worms.push_back({drop.def});
            fmt::print("  {} dropped!\n", drop.def->name);
        }
    }
}

bool pickup_worm(EntityComponentRegistry& reg, World& world, Entity player, int pickup_index) {
    auto* pos = reg.getComponent<Position>(player);
    auto* aperture = reg.getComponent<Aperture>(player);
    Map* map = world.get_map(pos->map_id);

    if (pickup_index < 0 || pickup_index >= (int)map->dropped_worms.size()) {
        fmt::print("No worm at that index.\n");
        return false;
    }
    if ((int)aperture->worms.size() >= aperture->capacity) {
        fmt::print("Your aperture is full ({} slots).\n", aperture->capacity);
        return false;
    }

    GuWorm worm = map->dropped_worms[pickup_index];
    aperture->worms.push_back(worm);
    map->dropped_worms.erase(map->dropped_worms.begin() + pickup_index);
    fmt::print("You refine {} into your aperture.\n", worm.def->name);
    return true;
}

bool drop_worm(EntityComponentRegistry& reg, World& world, Entity player, int drop_index) {
    auto* pos = reg.getComponent<Position>(player);
    auto* aperture = reg.getComponent<Aperture>(player);
    Map* map = world.get_map(pos->map_id);

    if (drop_index < 0 || drop_index >= (int)aperture->worms.size()) {
        fmt::print("No worm in that slot.\n");
        return false;
    }

    GuWorm worm = aperture->worms[drop_index];
    aperture->worms.erase(aperture->worms.begin() + drop_index);
    map->dropped_worms.push_back(worm);
    fmt::print("You release {} from your aperture.\n", worm.def->name);
    return true;
}
