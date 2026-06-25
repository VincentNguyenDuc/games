#include "systems/produce_system.hpp"

#include "components/boost.hpp"
#include "components/resource_producer.hpp"

#include <algorithm>

ProduceSystem::ProduceSystem() {
    writes = {typeid(ResourceProducer)};
    reads = {typeid(Boost)};
}

void ProduceSystem::update(ecse::EntityComponentRegistry& reg, ecse::CommandBuffer&) {
    if (dt <= 0.f)
        return;

    for (ecse::Entity e : reg.view<ResourceProducer>()) {
        auto* prod = reg.getComponent<ResourceProducer>(e);
        if (!prod)
            continue;

        float mult = 1.f;
        if (auto* b = reg.getComponent<Boost>(e))
            mult = b->multiplier;

        float gained = prod->rate_per_hour * mult * (dt / 3600.f);
        prod->stored = std::min(prod->stored + gained, prod->capacity);
    }
}
