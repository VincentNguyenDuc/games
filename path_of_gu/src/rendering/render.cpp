#include "rendering/render.hpp"

#include "components/aperture.hpp"
#include "components/cultivation_rank.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"

#include <ftxui/dom/elements.hpp>

using namespace ftxui;

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

static Element cell_element(int cell_type, char occupant) {
    std::string s{occupant, ' '};
    switch (occupant) {
    case '@':
        return text(s) | color(Color::GreenLight) | bold;
    case 'D':
        return text(s) | color(Color::Yellow) | bold;
    default:
        break;
    }
    if (occupant >= '1' && occupant <= '9')
        return text(s) | color(Color::Red) | bold;

    switch (cell_type) {
    case 1:
        return text(s) | color(Color::GrayDark);
    case 2:
        return text(s) | color(Color::Yellow) | bold;
    default:
        return text(s) | color(Color::BlueLight);
    }
}

static Element build_grid(
    EntityComponentRegistry& reg, GameWorld& world, Entity player, Map* map, const Position* pos
) {
    char display[10][10];
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x) {
            switch (map->grid[y][x]) {
            case 1:
                display[y][x] = '#';
                break;
            case 2:
                display[y][x] = 'D';
                break;
            default:
                display[y][x] = '.';
                break;
            }
        }

    int num = 1;
    for (Entity e : map->entities) {
        if (e == player)
            continue;
        auto* epos = reg.getComponent<Position>(e);
        auto* hp = reg.getComponent<Health>(e);
        if (epos && hp && hp->hp > 0 && num <= 9)
            display[epos->y][epos->x] = static_cast<char>('0' + num++);
    }
    display[pos->y][pos->x] = '@';

    Elements rows;
    for (int y = 0; y < 10; ++y) {
        Elements row;
        for (int x = 0; x < 10; ++x)
            row.push_back(cell_element(map->grid[y][x], display[y][x]));
        rows.push_back(hbox(std::move(row)));
    }
    return vbox(std::move(rows)) | border;
}

static Element build_info_panel(EntityComponentRegistry& reg, Map* map, Entity player) {
    Elements lines;
    lines.push_back(text("Enemies:") | bold);
    int num = 1;
    bool any_enemy = false;
    for (Entity e : map->entities) {
        if (e == player)
            continue;
        auto* hp = reg.getComponent<Health>(e);
        auto* name = reg.getComponent<Name>(e);
        if (hp && name) {
            any_enemy = true;
            lines.push_back(hbox({
                text(std::string("[") + char('0' + num++) + "] ") | color(Color::Red),
                text(name->value + "  "),
                text("HP ") | color(Color::GrayLight),
                text(std::to_string(hp->hp) + "/" + std::to_string(hp->max_hp)) |
                    color(hp->hp < hp->max_hp / 2 ? Color::Red : Color::Green),
            }));
        }
    }
    if (!any_enemy)
        lines.push_back(text("  (none)") | color(Color::GrayDark));

    lines.push_back(separator());
    lines.push_back(text("Worms on floor:") | bold);
    if (map->dropped_worms.empty()) {
        lines.push_back(text("  (none)") | color(Color::GrayDark));
    } else {
        for (int i = 0; i < (int)map->dropped_worms.size(); ++i) {
            const auto& def = *map->dropped_worms[i].def;
            lines.push_back(hbox({
                text("[" + std::to_string(i + 1) + "] ") | color(Color::Yellow),
                text(def.name + "  "),
                text(
                    std::string(worm_type_label(def.type)) + " +" + std::to_string(def.effect_value)
                ) | color(Color::Cyan),
            }));
        }
    }
    return vbox(std::move(lines)) | border | flex;
}

static Element build_stats_panel(EntityComponentRegistry& reg, Entity player, const Position* pos) {
    auto* hp = reg.getComponent<Health>(player);
    auto* essence = reg.getComponent<PrimevalEssence>(player);
    auto* rank = reg.getComponent<CultivationRank>(player);
    auto* name = reg.getComponent<Name>(player);
    auto* ap = reg.getComponent<Aperture>(player);

    Color hp_color = hp->hp < hp->max_hp / 3       ? Color::Red
                     : hp->hp < hp->max_hp * 2 / 3 ? Color::Yellow
                                                   : Color::Green;

    Elements stat_line = {
        text(name->value) | bold | color(Color::GreenLight),
        text("  │  HP "),
        text(std::to_string(hp->hp) + "/" + std::to_string(hp->max_hp)) | color(hp_color),
        text("  │  Essence "),
        text(std::to_string(essence->current) + "/" + std::to_string(essence->max)) |
            color(Color::Cyan),
        text("  │  Rank "),
        text(std::to_string(rank->rank)) | bold | color(Color::Yellow),
        text("  │  (" + std::to_string(pos->x) + "," + std::to_string(pos->y) + ")") |
            color(Color::GrayLight),
    };

    Elements worm_lines;
    worm_lines.push_back(
        text(
            "Aperture (" + std::to_string((int)ap->worms.size()) + "/" +
            std::to_string(ap->capacity) + " slots):"
        ) |
        bold
    );
    for (int i = 0; i < (int)ap->worms.size(); ++i) {
        const auto& def = *ap->worms[i].def;
        worm_lines.push_back(hbox({
            text("[" + std::to_string(i + 1) + "] ") | color(Color::Yellow),
            text(def.name + "  "),
            text(std::string(worm_type_label(def.type)) + " +" + std::to_string(def.effect_value)) |
                color(Color::Cyan),
            text("  cost " + std::to_string(def.essence_cost)) | color(Color::GrayLight),
            text("  range " + std::to_string(def.range)) | color(Color::GrayLight),
        }));
    }
    return vbox({hbox(std::move(stat_line)), vbox(std::move(worm_lines))}) | border;
}

ftxui::Element render(
    EntityComponentRegistry& reg, GameWorld& world, Entity player, const std::string& status_msg
) {
    auto* pos = reg.getComponent<Position>(player);
    Map* map = world.get_map(pos->map_id);

    auto header = vbox({
        text(map->name) | bold | color(Color::GreenLight) | hcenter,
        text(map->description) | color(Color::GrayLight) | hcenter,
    });

    auto hint = text("↑↓←→ move  ·  attack N  ·  use N (heal/buff)  ·  pickup N  ·  drop N  ·  "
                     "space skip  ·  quit") |
                color(Color::GrayDark) | hcenter;

    Element status_line = status_msg.empty() ? text("") : text(status_msg) | color(Color::Yellow);

    return vbox({
        header,
        separator(),
        hbox({build_grid(reg, world, player, map, pos), build_info_panel(reg, map, player)}),
        build_stats_panel(reg, player, pos),
        separator(),
        status_line,
        hint,
    });
}
