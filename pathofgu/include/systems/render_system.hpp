#pragma once

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "world/world.hpp"

#include <ftxui/dom/elements.hpp>
#include <string>

ftxui::Element render(
    EntityComponentRegistry& reg, World& world, Entity player, const std::string& status_msg
);
