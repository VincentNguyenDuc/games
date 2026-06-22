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
struct PickupCommand {
    int drop_index;
};                     // index into the map's dropped worms
struct SkipCommand {}; // pass the turn
struct QuitCommand {};

using PlayerCommand =
    std::variant<MoveCommand, AttackCommand, PickupCommand, SkipCommand, QuitCommand>;
