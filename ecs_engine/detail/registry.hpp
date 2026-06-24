#pragma once

#include "entity.hpp"

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

using ComponentType = std::type_index;

struct IComponentStore {
    virtual ~IComponentStore() = default;
};

// Sparse set: O(1) add/get/remove, components packed contiguously in `dense`.
template <typename T>
struct ComponentStore : IComponentStore {
    static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();

    std::vector<uint32_t> sparse; // sparse[entity] → index into dense, or INVALID
    std::vector<T> dense;         // packed component values
    std::vector<Entity> entities; // parallel to dense: which entity owns dense[i]

    void add(Entity e, T component) {
        if (e >= sparse.size())
            sparse.resize(e + 1, INVALID);
        if (sparse[e] != INVALID) {
            dense[sparse[e]] = std::move(component); // update in place
            return;
        }
        sparse[e] = static_cast<uint32_t>(dense.size());
        dense.push_back(std::move(component));
        entities.push_back(e);
    }

    T* get(Entity e) {
        if (e >= sparse.size() || sparse[e] == INVALID)
            return nullptr;
        return &dense[sparse[e]];
    }

    void remove(Entity e) {
        if (e >= sparse.size() || sparse[e] == INVALID)
            return;
        uint32_t idx = sparse[e];
        uint32_t last = static_cast<uint32_t>(dense.size()) - 1;
        // Swap-with-last to keep dense packed (no holes).
        if (idx != last) {
            dense[idx] = std::move(dense[last]);
            entities[idx] = entities[last];
            sparse[entities[idx]] = idx;
        }
        dense.pop_back();
        entities.pop_back();
        sparse[e] = INVALID;
    }
};

class EntityComponentRegistry {
    std::unordered_map<ComponentType, std::unique_ptr<IComponentStore>> stores_;

    template <typename T>
    ComponentStore<T>& store() {
        auto& ptr = stores_[std::type_index(typeid(T))];
        if (!ptr)
            ptr = std::make_unique<ComponentStore<T>>();
        return *static_cast<ComponentStore<T>*>(ptr.get());
    }

public:
    template <typename T>
    void addComponent(Entity entity, T component) {
        store<T>().add(entity, std::move(component));
    }

    template <typename T>
    T* getComponent(Entity entity) {
        return store<T>().get(entity);
    }

    template <typename T>
    void removeComponent(Entity entity) {
        store<T>().remove(entity);
    }

    // Returns a snapshot of entities that have T — safe to iterate while removing.
    template <typename T>
    std::vector<Entity> view() {
        return store<T>().entities;
    }
};
