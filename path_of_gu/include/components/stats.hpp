#pragma once

struct Stats {
    int base_attack;
    int base_defense;     // flat damage reduction per hit (minimum 1 always gets through)
    int attack_range = 1; // Chebyshev range for AI attacks
};
