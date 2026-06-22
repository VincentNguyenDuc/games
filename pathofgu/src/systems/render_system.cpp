#include "systems/render_system.hpp"

#include "components/aperture.hpp"
#include "components/cultivation_rank.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"

#include <array>
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

    // Build display grid from cell types
    std::array<std::array<char, 10>, 10> display{};
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x) {
            switch (map->grid[y][x]) {
            case Cell::Wall:
                display[y][x] = '#';
                break;
            case Cell::Door:
                display[y][x] = 'D';
                break;
            default:
                display[y][x] = '.';
                break;
            }
        }

    // Overlay enemies (numbered 1–9)
    int enemy_num = 1;
    std::vector<std::pair<Entity, int>> enemy_list; // entity + display number
    for (Entity e : map->entities) {
        if (e == player)
            continue;
        auto* epos = reg.getComponent<Position>(e);
        auto* hp = reg.getComponent<Health>(e);
        if (epos && hp && hp->hp > 0 && enemy_num <= 9) {
            display[epos->y][epos->x] = static_cast<char>('0' + enemy_num);
            enemy_list.emplace_back(e, enemy_num++);
        }
    }

    // Overlay player
    display[pos->y][pos->x] = '@';

    // Print grid
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x)
            fmt::print("{} ", display[y][x]);
        fmt::print("\n");
    }
    fmt::print("Legend: # wall  . floor  D door  @ you  1-9 enemy\n\n");

    // Enemy list
    for (auto [e, num] : enemy_list) {
        auto* hp = reg.getComponent<Health>(e);
        auto* name = reg.getComponent<Name>(e);
        fmt::print("  [{}] {:25s}  HP: {}/{}\n", num, name->value, hp->hp, hp->max_hp);
    }
    if (enemy_list.empty())
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
        "{} | HP: {}/{} | Essence: {}/{} | Rank {} | pos ({},{})\n",
        name->value,
        hp->hp,
        hp->max_hp,
        essence->current,
        essence->max,
        rank->rank,
        pos->x,
        pos->y
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
    fmt::print("attack <slot>  |  w/a/s/d  |  pickup <#>  |  drop <#>  |  skip  |  quit\n> ");
}
