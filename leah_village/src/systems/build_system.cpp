#include "systems/build_system.hpp"

#include "components/construction.hpp"

ConstructionSystem::ConstructionSystem() { writes = {typeid(Construction)}; }

void ConstructionSystem::update(ecse::EntityComponentRegistry& reg, ecse::CommandBuffer& cmd) {
    if (dt <= 0.f)
        return;

    for (ecse::Entity e : reg.view<Construction>()) {
        auto* c = reg.getComponent<Construction>(e);
        if (!c)
            continue;

        c->time_remaining_s -= dt;
        if (c->time_remaining_s <= 0.f) {
            bool is_upgrade = (c->kind == ConstructionKind::Upgrade);
            cmd.remove_component<Construction>(e);
            if (on_complete)
                on_complete(e, is_upgrade);
        }
    }
}
