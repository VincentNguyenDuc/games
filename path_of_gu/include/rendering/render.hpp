#pragma once

#include "ecs.hpp"
#include "world/world.hpp"

#include <ftxui/dom/elements.hpp>
#include <string>

using ecse::Entity;
using ecse::EntityComponentRegistry;

ftxui::Element render(
    EntityComponentRegistry& reg, World& world, Entity player, const std::string& status_msg
);
