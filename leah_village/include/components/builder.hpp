#pragma once

#include "entity.hpp"

using ecse::Entity;

// One Builder component per Builder's Hut entity.
// When busy, `assigned_to` is the building entity under construction.
struct Builder {
    bool busy{false};
    Entity assigned_to{0};
};
