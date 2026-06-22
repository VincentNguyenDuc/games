#include "systems/render_system.hpp"

#include "components/aperture.hpp"
#include "components/cultivation_rank.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"

#include <fmt/format.h>

static const char* worm_type_label(GuWormType t) {
    switch (t) {
    case GuWormType::Offensive:
        return "ATK";
    case GuWormType::Defensive:
        return "DEF";
    case GuWormType::Recovery:
        return "HEAL";
    case GuWormType::Movement:
        return "MOV";
    case GuWormType::Support:
        return "SUP";
    }
    return "???";
}

void render(EntityComponentRegistry& reg, World& world, Entity player) {
    auto* pos = reg.getComponent<Position>(player);
    Map* map = world.get_map(pos->map_id);

    fmt::print("\n===========================================\n");
    fmt::print("  {}\n", map->name);
    fmt::print("===========================================\n");
    fmt::print("{}\n\n", map->description);

    // Exits
    std::string exits_str;
    for (const auto& [dir, _] : map->exits)
        exits_str += dir + "  ";
    fmt::print("Exits: {}\n\n", exits_str.empty() ? "(none)" : exits_str);

    // Enemies in map
    int enemy_idx = 1;
    for (Entity e : map->entities) {
        if (e == player)
            continue;
        auto* hp = reg.getComponent<Health>(e);
        auto* name = reg.getComponent<Name>(e);
        if (hp && name)
            fmt::print("  [{}] {:25s}  HP: {}/{}\n", enemy_idx++, name->value, hp->hp, hp->max_hp);
    }
    if (enemy_idx == 1)
        fmt::print("  (no enemies)\n");

    // Dropped worms
    fmt::print("\nDropped worms:\n");
    if (map->dropped_worms.empty()) {
        fmt::print("  (none)\n");
    } else {
        for (int i = 0; i < (int)map->dropped_worms.size(); ++i) {
            const auto& def = *map->dropped_worms[i].def;
            fmt::print(
                "  [{}] {:25s}  Rank {}  {} +{}\n",
                i + 1,
                def.name,
                def.rank,
                worm_type_label(def.type),
                def.effect_value
            );
        }
    }

    // Player status
    auto* hp = reg.getComponent<Health>(player);
    auto* essence = reg.getComponent<PrimevalEssence>(player);
    auto* rank = reg.getComponent<CultivationRank>(player);
    auto* name = reg.getComponent<Name>(player);
    auto* ap = reg.getComponent<Aperture>(player);

    fmt::print("\n-------------------------------------------\n");
    fmt::print(
        "{} | HP: {}/{} | Essence: {}/{} | Rank {}\n",
        name->value,
        hp->hp,
        hp->max_hp,
        essence->current,
        essence->max,
        rank->rank
    );
    fmt::print("Aperture ({}/{} slots):\n", (int)ap->worms.size(), ap->capacity);
    for (int i = 0; i < (int)ap->worms.size(); ++i) {
        const auto& def = *ap->worms[i].def;
        fmt::print(
            "  [{}] {:25s}  Rank {}  {} +{:3d}   Cost: {} essence\n",
            i + 1,
            def.name,
            def.rank,
            worm_type_label(def.type),
            def.effect_value,
            def.essence_cost
        );
    }
    fmt::print("-------------------------------------------\n");
    fmt::print("attack <slot>  |  move <n/s/e/w>  |  pickup <#>  |  skip  |  quit\n> ");
}
