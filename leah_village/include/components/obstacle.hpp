#pragma once

// Clearable terrain feature (tree, rock, bush) that blocks building placement.
// When a clear job is started, time_remaining_s counts down to 0.
// Grants resources to the player on completion.
struct Obstacle {
    int gold_reward;
    int elixir_reward;
    float clear_time_s{60.f};    // total time needed to clear
    float time_remaining_s{0.f}; // 0 = not being cleared; >0 = in progress
};
