#include "systems/combat_system.hpp"
#include "components/aperture.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/stats.hpp"
#include "utility.hpp"

#include <fmt/format.h>

CombatResult player_attack(
    EntityComponentRegistry& reg, Entity player, Entity target, int worm_slot
) {
    auto* aperture = reg.getComponent<Aperture>(player);
    auto* essence = reg.getComponent<PrimevalEssence>(player);
    auto* def_hp = reg.getComponent<Health>(target);
    auto* def_name = reg.getComponent<Name>(target);
    auto* def_stats = reg.getComponent<Stats>(target);

    if (!aperture || !essence || !def_hp)
        return {false, "Invalid target."};

    if (worm_slot < 0 || worm_slot >= (int)aperture->worms.size())
        return {false, "No worm in that slot."};

    const auto& worm = aperture->worms[worm_slot];
    const auto& def = *worm.def;

    // Range check — range 0 means self-targeting (Recovery, Support, Movement): skip.
    if (def.range > 0) {
        auto* ppos = reg.getComponent<Position>(player);
        auto* tpos = reg.getComponent<Position>(target);
        if (ppos && tpos) {
            int dist = chebyshev(*ppos, *tpos);
            if (dist > def.range)
                return {
                    false,
                    fmt::format(
                        "{} is out of range (distance {}, {} range {}).",
                        def_name ? def_name->value : "Target",
                        dist,
                        def.name,
                        def.range
                    )};
        }
    }

    if (essence->current < def.essence_cost)
        return {false, "Not enough primeval essence to activate " + def.name + "."};

    essence->current -= def.essence_cost;

    int damage = 0;
    std::string effect_msg;

    switch (def.type) {
    case GuWormType::Offensive: {
        int defense = def_stats ? std::max(0, def_stats->base_defense) : 0;
        damage = std::max(1, def.effect_value - defense);
        def_hp->hp -= damage;
        effect_msg = fmt::format("deals {} damage", damage);
        break;
    }
    case GuWormType::Defensive: {
        damage = std::max(1, def.effect_value / 2);
        def_hp->hp -= damage;
        effect_msg = fmt::format("deflects and strikes for {} damage", damage);
        break;
    }

    case GuWormType::Recovery: {
        auto* player_hp = reg.getComponent<Health>(player);
        int healed = std::min(def.effect_value, player_hp->max_hp - player_hp->hp);
        player_hp->hp += healed;
        effect_msg = fmt::format("restores {} HP", healed);
        break;
    }
    case GuWormType::Movement:
    case GuWormType::Support:
        effect_msg = "has no combat effect here";
        break;
    }

    bool died = def_hp->hp <= 0;
    std::string msg = fmt::format(
        "You activate {}! It {} on {}.",
        def.name,
        effect_msg,
        def_name ? def_name->value : "the enemy"
    );
    if (died)
        msg += fmt::format("\n{} has been defeated!", def_name ? def_name->value : "Enemy");

    return {true, msg, died, damage};
}

CombatResult player_use(EntityComponentRegistry& reg, Entity player, int worm_slot) {
    auto* aperture = reg.getComponent<Aperture>(player);
    auto* essence = reg.getComponent<PrimevalEssence>(player);
    auto* hp = reg.getComponent<Health>(player);

    if (!aperture || !essence || !hp)
        return {false, "Invalid state."};
    if (worm_slot < 0 || worm_slot >= (int)aperture->worms.size())
        return {false, "No worm in that slot."};

    const auto& worm = aperture->worms[worm_slot];
    const auto& def = *worm.def;

    if (def.range > 0)
        return {
            false,
            fmt::format(
                "{} is an offensive worm — use 'attack {}' to target an enemy.",
                def.name,
                worm_slot + 1
            )};

    if (essence->current < def.essence_cost)
        return {false, "Not enough primeval essence to activate " + def.name + "."};

    essence->current -= def.essence_cost;

    std::string effect_msg;
    switch (def.type) {
    case GuWormType::Recovery: {
        int healed = std::min(def.effect_value, hp->max_hp - hp->hp);
        hp->hp += healed;
        effect_msg = fmt::format("restores {} HP (now {}/{})", healed, hp->hp, hp->max_hp);
        break;
    }
    case GuWormType::Support:
    case GuWormType::Movement:
        effect_msg = "pulses with energy (no immediate effect)";
        break;
    default:
        effect_msg = "activates";
        break;
    }

    return {true, fmt::format("You activate {}! It {}.", def.name, effect_msg)};
}
