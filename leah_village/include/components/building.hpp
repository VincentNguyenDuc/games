#pragma once

enum class BuildingType {
    TownHall,
    GoldMine,
    ElixirCollector,
    GoldStorage,
    ElixirStorage,
    BuilderHut,
    Farm,
    Decoration,
};

struct Building {
    BuildingType type;
    int size; // footprint: size × size tiles
    int max_hp;
    int hp;
};
