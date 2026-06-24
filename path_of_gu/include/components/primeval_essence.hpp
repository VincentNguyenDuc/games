#pragma once

struct PrimevalEssence {
    int current;
    int max;
    int depleted_turns = 0; // tracks consecutive turns at 0 (lose at 3)
};
