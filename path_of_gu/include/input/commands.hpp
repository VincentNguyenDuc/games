#pragma once

#include <string>
#include <variant>

// Commands produced by the input thread and consumed by the game loop.
// Each type represents one thing the player can do on their turn.

struct MoveCommand {
    std::string direction;
}; // "north", "south", "east", "west"
struct AttackCommand {
    int worm_slot;
}; // 0-indexed slot in the player's aperture
struct HealCommand {
    int worm_slot;
};
struct PickupCommand {
    int pickup_index;
}; // index into the map's dropped worms
struct DropCommand {
    int pickup_index;
};
struct SkipCommand {}; // pass the turn
struct QuitCommand {};

using PlayerCommand = std::variant<
    MoveCommand,
    AttackCommand,
    HealCommand,
    PickupCommand,
    DropCommand,
    SkipCommand,
    QuitCommand>;
