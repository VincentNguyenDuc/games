#pragma once

#include "components/ai_behavior.hpp"
#include "ecs.hpp"
#include "events/events.hpp"
#include "items/gu_worm_db.hpp"
#include "systems/ai_system.hpp"
#include "systems/effect_system.hpp"
#include "systems/movement_system.hpp"
#include "world/world.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <string>

class Game {
    GameWorld game_world_;
    World ecs_world_;
    std::unique_ptr<IGuWormDatabase> db_;
    Entity player_;
    ftxui::ScreenInteractive screen_ = ftxui::ScreenInteractive::Fullscreen();
    std::string status_msg_;

    // Non-owning — systems are owned by ecs_world_.
    AiTickSystem* ai_sys_{nullptr};
    MoveTickSystem* move_sys_{nullptr};
    SelfEffectTickSystem* self_eff_sys_{nullptr};
    AttackEffectTickSystem* atk_eff_sys_{nullptr};

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
    void cleanup_dead();
    void destroy_entity(Entity e);
    bool is_in_combat() const;
    bool check_win();
};
