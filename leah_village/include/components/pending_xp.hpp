#pragma once

// Transient: attached to any entity to award XP to the player on the next tick.
// LevelUpSystem consumes these and removes the component.
struct PendingXP {
    int amount;
};
