#include "actions.hpp"
#include "assets.hpp"
#include "game.hpp"

// ─── Entity spawning ─────────────────────────────────────────────────────────

ecse::Entity Actions::spawn_building(MapId map_id, int x, int y, BuildingType type, int level) {
    const auto& d = Assets::def(type);
    const auto& stat = d.stats[level - 1];

    ecse::Entity e = g_.ecs_.entities().createEntity();
    g_.ecs_.registry().addComponent<Position>(e, {x, y});
    g_.ecs_.registry().addComponent<MapLocation>(e, {map_id});
    g_.ecs_.registry().addComponent<Name>(e, {std::string(d.name)});
    g_.ecs_.registry().addComponent<Building>(e, {type, 1, stat.hp, stat.hp});
    g_.ecs_.registry().addComponent<Level>(e, {level, d.max_level});

    switch (type) {
    case BuildingType::GoldMine:
        g_.ecs_.registry().addComponent<ResourceProducer>(
            e, {ResourceType::Gold, stat.rate_per_hour, 0.f, stat.prod_cap}
        );
        break;
    case BuildingType::ElixirCollector:
        g_.ecs_.registry().addComponent<ResourceProducer>(
            e, {ResourceType::Elixir, stat.rate_per_hour, 0.f, stat.prod_cap}
        );
        break;
    case BuildingType::GoldStorage:
        g_.ecs_.registry().addComponent<ResourceStorage>(
            e, {ResourceType::Gold, 0, stat.storage_cap}
        );
        break;
    case BuildingType::ElixirStorage:
        g_.ecs_.registry().addComponent<ResourceStorage>(
            e, {ResourceType::Elixir, 0, stat.storage_cap}
        );
        break;
    case BuildingType::BuilderHut:
        g_.ecs_.registry().addComponent<Builder>(e, {false, 0});
        break;
    case BuildingType::Farm:
        g_.ecs_.registry().addComponent<PopulationProvider>(e, {stat.pop_cap});
        break;
    default:
        break;
    }

    if (level < d.max_level) {
        const auto& next = d.stats[level];
        g_.ecs_.registry().addComponent<UpgradeCost>(e, {next.gold_cost, next.elixir_cost});
    }

    Map& m = g_.world_.get_map(map_id);
    m.place_entity(x, y, e);
    return e;
}

ecse::Entity Actions::spawn_obstacle(
    MapId map_id, int x, int y, int gold, int elixir, float clear_time_s
) {
    ecse::Entity e = g_.ecs_.entities().createEntity();
    g_.ecs_.registry().addComponent<Position>(e, {x, y});
    g_.ecs_.registry().addComponent<MapLocation>(e, {map_id});
    g_.ecs_.registry().addComponent<Obstacle>(e, {gold, elixir, clear_time_s, 0.f});

    g_.world_.get_map(map_id).place_entity(x, y, e);
    return e;
}

void Actions::spawn_initial_buildings() {
    // ── Home Village (Map 0) ──────────────────────────────────────────────────
    spawn_building(0, 10, 5, BuildingType::TownHall);
    spawn_building(0, 4, 3, BuildingType::GoldMine);
    spawn_building(0, 16, 3, BuildingType::ElixirCollector);
    spawn_building(0, 4, 9, BuildingType::GoldStorage);
    spawn_building(0, 16, 9, BuildingType::ElixirStorage);
    spawn_building(0, 2, 9, BuildingType::BuilderHut);
    spawn_building(0, 18, 9, BuildingType::Farm);

    spawn_obstacle(0, 7, 2, 25, 0, 5.f);
    spawn_obstacle(0, 13, 2, 0, 25, 5.f);
    spawn_obstacle(0, 7, 8, 30, 0, 5.f);
    spawn_obstacle(0, 13, 8, 0, 30, 5.f);

    // ── Ancient Forest (Map 1) ───────────────────────────────────────────────
    spawn_building(1, 5, 5, BuildingType::ElixirCollector);
    spawn_building(1, 13, 5, BuildingType::ElixirCollector);
    spawn_building(1, 9, 8, BuildingType::Farm);

    for (auto [ox, oy] : std::initializer_list<std::pair<int, int>>{
             {2, 2}, {3, 7}, {7, 3}, {11, 8}, {15, 2}, {15, 7}, {6, 9}, {12, 3}})
        spawn_obstacle(1, ox, oy, 0, 40, 10.f);

    // ── Crystal Caves (Map 2) ────────────────────────────────────────────────
    spawn_building(2, 6, 4, BuildingType::GoldMine, 2);
    spawn_building(2, 18, 4, BuildingType::GoldMine, 2);
    spawn_building(2, 12, 8, BuildingType::GoldStorage, 2);
    spawn_building(2, 4, 11, BuildingType::Farm);

    for (auto [ox, oy] : std::initializer_list<std::pair<int, int>>{
             {9, 3}, {14, 3}, {3, 6}, {21, 6}, {9, 12}, {14, 12}})
        spawn_obstacle(2, ox, oy, 100, 0, 15.f);
}
