#pragma once

#include "ecs.hpp"
#include "world/world.hpp"

#include <ftxui/dom/elements.hpp>
#include <string>

ftxui::Element render(
    EntityComponentRegistry& reg, GameWorld& world, Entity player, const std::string& status_msg
);
