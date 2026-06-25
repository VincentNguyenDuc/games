#pragma once

#include "ecs.hpp"
#include "world/world.hpp"

#include <string>

// Forward declaration — Game passes itself in.
class Game;

namespace Persistence {

// Returns true if the save file exists and was loaded successfully.
bool load(const std::string& path, Game& game);

// Overwrites (or creates) the save file.
void save(const std::string& path, const Game& game);

} // namespace Persistence
