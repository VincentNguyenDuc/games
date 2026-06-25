#pragma once

#include "components/building.hpp"
#include "components/construction.hpp"
#include "ecs.hpp"
#include "world/map.hpp"

class Game;

class Actions {
    Game& g_;

public:
    explicit Actions(Game& g);

    // Spawn
    ecse::Entity spawn_building(MapId map_id, int x, int y, BuildingType type, int level = 1);
    ecse::Entity spawn_obstacle(
        MapId map_id, int x, int y, int gold, int elixir, float clear_time_s
    );
    void spawn_initial_buildings();

    // Player
    void try_move(int dx, int dy);
    void try_interact();
    void try_upgrade();
    void try_clear();
    void enter_build_mode();
    void confirm_build();
    void cancel_build();

    // Callbacks / internal
    void do_collect(ecse::Entity e);
    void do_clear_obstacle(ecse::Entity e);
    void start_construction(ecse::Entity e, ConstructionKind kind, float time_s);
    void finish_construction(ecse::Entity e, bool is_upgrade);
    void finish_obstacle(ecse::Entity e);
    void on_level_up(int new_level);
};
