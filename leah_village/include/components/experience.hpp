#pragma once

// On the player entity. Village-wide XP and level tracking.
struct Experience {
    int level{1};
    int current_xp{0};
    int xp_to_next{100};
};
