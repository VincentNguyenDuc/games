#pragma once

#include <cstdint>
#include <limits>
#include <queue>

using Entity = std::uint16_t;
const Entity ENTITY_LIMIT = std::numeric_limits<Entity>::max();

class EntityManager {
private:
    std::queue<Entity> recycle;
    Entity nextEntity = 0;

public:
    EntityManager();
    Entity createEntity();
    void destroyEntity(Entity entity);
};
