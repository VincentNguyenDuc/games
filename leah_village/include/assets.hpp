#pragma once

#include "components/building.hpp"

#include <string_view>

namespace Assets {

struct LevelStat {
    int hp;
    int gold_cost;
    int elixir_cost;
    int build_time_s;
    float rate_per_hour{0.f}; // producers: resource units per hour
    float prod_cap{0.f};      // producers: max stored before full
    int storage_cap{0};       // storages: max held
    int pop_cap{0};           // farms: population slots provided
};

struct BuildingDef {
    std::string_view name;
    std::string_view symbol; // 2-char terminal symbol
    int max_level;
    LevelStat stats[10]; // index 0 = level 1
};

// Indexed by BuildingType enum value.
extern const BuildingDef DEFS[8];

inline const BuildingDef& def(BuildingType t) { return DEFS[static_cast<int>(t)]; }

// XP required to reach each player level (index = level, so [1] = XP to reach level 2).
extern const int XP_TABLE[16];

inline int xp_to_reach(int level) {
    if (level < 1 || level > 15)
        return 999999;
    return XP_TABLE[level];
}

// Unlock thresholds
inline int map1_level() { return 3; } // Ancient Forest
inline int map2_level() { return 7; } // Crystal Caves

// Builder slots granted at each player level
inline int builder_slots_for_level(int level) {
    if (level >= 10)
        return 4;
    if (level >= 5)
        return 3;
    if (level >= 3)
        return 2;
    return 1;
}

} // namespace Assets
