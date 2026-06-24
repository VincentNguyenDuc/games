#pragma once

#include "entity.hpp"
#include "registry.hpp"

#include <functional>
#include <vector>

class CommandBuffer {
    using Command = std::function<void(EntityComponentRegistry&, EntityManager&)>;
    std::vector<Command> commands_;

public:
    template <typename T>
    void add_component(Entity e, T component) {
        commands_.push_back(
            [e, c = std::move(component)](EntityComponentRegistry& reg, EntityManager&) mutable {
            reg.addComponent(e, std::move(c));
        });
    }

    template <typename T>
    void remove_component(Entity e) {
        commands_.push_back([e](EntityComponentRegistry& reg, EntityManager&) {
            reg.removeComponent<T>(e);
        });
    }

    void destroy_entity(Entity e) {
        commands_.push_back([e](EntityComponentRegistry&, EntityManager& em) {
            em.destroyEntity(e);
        });
    }

    void flush(EntityComponentRegistry& reg, EntityManager& em) {
        for (auto& cmd : commands_)
            cmd(reg, em);
        commands_.clear();
    }
};
