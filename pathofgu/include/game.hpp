#pragma once

#include "components/ai_behavior.hpp"
#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "events/event_queue.hpp"
#include "events/events.hpp"
#include "items/gu_worm_db.hpp"
#include "world/world.hpp"

#include <memory>
#include <thread>

class Game {
    std::shared_ptr<EntityComponentRegistry> reg_;
    std::unique_ptr<World> world_;
    std::unique_ptr<IGuWormDatabase> db_;
    EntityManager entity_manager_;
    EventQueue<PlayerCommand> queue_;
    Entity player_;
    bool running_ = true;

public:
    Game();
    void run();

private:
    void spawn_player();
    void spawn_enemies();
    Entity make_enemy(
        MapId map,
        const std::string& name,
        int hp,
        int base_attack,
        int base_defense,
        BehaviorType behavior,
        std::vector<GuWormDrop> drops
    );

    void process_command(const PlayerCommand& cmd);
    void post_action_tick();
    void check_deaths();
    void destroy_entity(Entity e);
    bool is_in_combat() const;
    bool check_win();
};
