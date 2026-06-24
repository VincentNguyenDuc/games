#include "ecs/entity.hpp"
#include <spdlog/spdlog.h>

EntityManager::EntityManager() {}

Entity EntityManager::createEntity() {
    if (!recycle.empty()) {
        spdlog::debug("Recycling entity: {}", recycle.front());
        Entity entity = recycle.front();
        recycle.pop();
        return entity;
    }

    if (nextEntity < ENTITY_LIMIT) {
        spdlog::debug("Creating new entity: {}", nextEntity);
        return nextEntity++;
    }
    throw std::runtime_error("No more available entities.");
}

void EntityManager::destroyEntity(Entity entity) { recycle.push(entity); }