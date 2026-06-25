#include "systems/extract_system.hpp"

#include "components/obstacle.hpp"

ExtractSystem::ExtractSystem() { writes = {typeid(Obstacle)}; }

void ExtractSystem::update(ecse::EntityComponentRegistry& reg, ecse::CommandBuffer& cmd) {
    if (dt <= 0.f)
        return;

    for (ecse::Entity e : reg.view<Obstacle>()) {
        auto* obs = reg.getComponent<Obstacle>(e);
        if (!obs || obs->time_remaining_s <= 0.f)
            continue;

        obs->time_remaining_s -= dt;
        if (obs->time_remaining_s <= 0.f) {
            obs->time_remaining_s = 0.f;
            if (on_cleared)
                on_cleared(e);
        }
    }
}
