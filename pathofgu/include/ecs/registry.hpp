#pragma once

#include "entity.hpp"

#include <any>
#include <typeindex>
#include <unordered_map>
#include <vector>

using ComponentType = std::type_index;
using Component = std::any;
using ComponentStorage = std::unordered_map<Entity, Component>;

class EntityComponentRegistry {
    std::unordered_map<ComponentType, ComponentStorage> registry;

public:
    template <typename T>
    void addComponent(Entity entity, T component) {
        registry[std::type_index(typeid(T))][entity] = std::move(component);
    }

    template <typename T>
    T* getComponent(Entity entity) {
        auto outer = registry.find(std::type_index(typeid(T)));
        if (outer == registry.end())
            return nullptr;
        auto inner = outer->second.find(entity);
        if (inner == outer->second.end())
            return nullptr;
        return std::any_cast<T>(&inner->second);
    }

    template <typename T>
    void removeComponent(Entity entity) {
        auto outer = registry.find(std::type_index(typeid(T)));
        if (outer == registry.end())
            return;
        outer->second.erase(entity);
    }

    template <typename T>
    std::vector<Entity> view() {
        std::vector<Entity> entities;
        auto outer = registry.find(std::type_index(typeid(T)));
        if (outer == registry.end())
            return entities;
        for (auto& [entity, _] : outer->second)
            entities.push_back(entity);
        return entities;
    }
};
