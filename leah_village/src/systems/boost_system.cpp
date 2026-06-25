#include "systems/boost_system.hpp"

#include "components/boost.hpp"

BoostSystem::BoostSystem() { writes = {typeid(Boost)}; }

void BoostSystem::update(ecse::EntityComponentRegistry& reg, ecse::CommandBuffer& cmd) {
    if (dt <= 0.f)
        return;

    for (ecse::Entity e : reg.view<Boost>()) {
        auto* b = reg.getComponent<Boost>(e);
        if (!b)
            continue;

        b->time_remaining_s -= dt;
        if (b->time_remaining_s <= 0.f)
            cmd.remove_component<Boost>(e);
    }
}
