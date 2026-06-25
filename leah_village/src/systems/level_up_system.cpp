#include "systems/level_up_system.hpp"

#include "assets.hpp"
#include "components/experience.hpp"
#include "components/pending_xp.hpp"

LevelUpSystem::LevelUpSystem() { writes = {typeid(Experience), typeid(PendingXP)}; }

void LevelUpSystem::update(ecse::EntityComponentRegistry& reg, ecse::CommandBuffer& cmd) {
    auto* exp = reg.getComponent<Experience>(player_entity);
    if (!exp)
        return;

    // Accumulate all pending XP grants into the player's experience.
    for (ecse::Entity e : reg.view<PendingXP>()) {
        auto* p = reg.getComponent<PendingXP>(e);
        if (!p)
            continue;
        exp->current_xp += p->amount;
        cmd.remove_component<PendingXP>(e);
    }

    // Level-up loop: keep levelling while XP exceeds threshold.
    while (exp->level < 15 && exp->current_xp >= exp->xp_to_next) {
        exp->current_xp -= exp->xp_to_next;
        exp->level++;
        exp->xp_to_next = Assets::xp_to_reach(exp->level);
        if (on_level_up)
            on_level_up(exp->level);
    }
}
