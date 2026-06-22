#include "input/input_thread.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

std::optional<PlayerCommand> parse_command(const std::string& line) {
    if (line.empty())
        return std::nullopt;

    std::istringstream ss(line);
    std::string verb;
    ss >> verb;

    // Normalise to lowercase
    std::transform(verb.begin(), verb.end(), verb.begin(), ::tolower);

    if (verb == "move" || verb == "go") {
        std::string dir;
        if (ss >> dir)
            return MoveCommand{dir};
        return std::nullopt;
    }

    // Directional shorthand: n/s/e/w
    if (verb == "n")
        return MoveCommand{"north"};
    if (verb == "s")
        return MoveCommand{"south"};
    if (verb == "e")
        return MoveCommand{"east"};
    if (verb == "w")
        return MoveCommand{"west"};

    if (verb == "attack" || verb == "use") {
        int slot = 0;
        if (ss >> slot)
            return AttackCommand{slot - 1}; // 1-based input → 0-based index
        return std::nullopt;
    }

    if (verb == "pickup" || verb == "take") {
        int idx = 0;
        if (ss >> idx)
            return PickupCommand{idx - 1};
        return std::nullopt;
    }

    if (verb == "skip" || verb == "wait")
        return SkipCommand{};
    if (verb == "quit" || verb == "exit")
        return QuitCommand{};

    return std::nullopt;
}

std::jthread launch_input_thread(EventQueue<PlayerCommand>& queue) {
    return std::jthread([&queue](std::stop_token stop) {
        std::string line;
        while (!stop.stop_requested() && std::getline(std::cin, line)) {
            if (auto cmd = parse_command(line))
                queue.push(std::move(*cmd));
        }
    });
}
