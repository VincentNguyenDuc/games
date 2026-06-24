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
    int level; // 1-based; increases on upgrade completion
    int size;  // footprint: size × size tiles
    int max_hp;
    int hp;
};
