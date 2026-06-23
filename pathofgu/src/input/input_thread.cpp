#include "input/input_thread.hpp"

#include <algorithm>
#include <sstream>

std::optional<PlayerCommand> parse_command(const std::string& line) {
    if (line.empty())
        return std::nullopt;

    std::istringstream ss(line);
    std::string verb;
    ss >> verb;

    std::transform(verb.begin(), verb.end(), verb.begin(), ::tolower);

    if (verb == "move" || verb == "go") {
        std::string dir;
        if (ss >> dir)
            return MoveCommand{dir};
        return std::nullopt;
    }

    // wasd shorthands
    if (verb == "w")
        return MoveCommand{"north"};
    if (verb == "s")
        return MoveCommand{"south"};
    if (verb == "d")
        return MoveCommand{"east"};
    if (verb == "a")
        return MoveCommand{"west"};

    if (verb == "attack") {
        int slot = 0;
        if (ss >> slot)
            return AttackCommand{slot - 1};
        return std::nullopt;
    }

    if (verb == "use") {
        int slot = 0;
        if (ss >> slot)
            return HealCommand{slot - 1};
        return std::nullopt;
    }

    if (verb == "pickup" || verb == "take") {
        int idx = 0;
        if (ss >> idx)
            return PickupCommand{idx - 1};
        return std::nullopt;
    }

    if (verb == "drop") {
        int idx = 0;
        if (ss >> idx)
            return DropCommand{idx - 1};
        return std::nullopt;
    }

    if (verb == "skip" || verb == "wait")
        return SkipCommand{};
    if (verb == "quit" || verb == "exit")
        return QuitCommand{};

    return std::nullopt;
}
