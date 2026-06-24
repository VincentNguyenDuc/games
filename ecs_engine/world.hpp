#pragma once

#include "detail/command_buffer.hpp"
#include "detail/entity.hpp"
#include "detail/registry.hpp"
#include "detail/system.hpp"
#include "detail/thread_pool.hpp"

#include <memory>
#include <vector>

class World {
    EntityManager entities_;
    EntityComponentRegistry registry_;
    SystemsScheduler scheduler_;
    ThreadPool pool_;
    std::vector<std::unique_ptr<ISystem>> systems_;
    std::vector<CommandBuffer> cmd_buffers_;

public:
    EntityManager& entities() { return entities_; }
    const EntityManager& entities() const { return entities_; }
    EntityComponentRegistry& registry() { return registry_; }
    const EntityComponentRegistry& registry() const { return registry_; }

    template <typename T, typename... Args>
    T& add_system(Args&&... args) {
        auto sys = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *sys;
        scheduler_.add_system(sys.get());
        systems_.push_back(std::move(sys));
        return ref;
    }

    // Call once after all systems are registered.
    void build() {
        scheduler_.build();
        cmd_buffers_.resize(systems_.size());
    }

    void tick();
};
