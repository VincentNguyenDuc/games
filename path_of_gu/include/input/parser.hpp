#pragma once

#include "events/events.hpp"

#include <optional>
#include <string>

// Parses a raw input line into a PlayerCommand.
// Returns nullopt for unrecognised input.
std::optional<PlayerCommand> parse_command(const std::string& line);
