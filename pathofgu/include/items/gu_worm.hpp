#pragma once

#include <memory>
#include <string>

enum class GuWormType { Offensive, Defensive, Recovery, Movement, Support };

struct GuWormDefinition {
    std::string name;
    int rank; // 1-5 (mortal grade)
    GuWormType type;
    int essence_cost; // primeval essence consumed per activation
    int effect_value; // damage / armor / heal / etc.
};

struct GuWorm {
    std::shared_ptr<GuWormDefinition> def;
};

struct GuWormDrop {
    std::shared_ptr<GuWormDefinition> def;
    float chance; // 0.0–1.0
};
