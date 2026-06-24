#include "actions/combat.hpp"

#include "components/aperture.hpp"
#include "components/attack_effect.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/self_effect.hpp"
#include "utility.hpp"

#include <fmt/format.h>

ActivationResult activate_worm(
    EntityComponentRegistry& reg, Entity source, Entity target, int worm_slot
) {
    auto* aperture = reg.getComponent<Aperture>(source);
    auto* essence = reg.getComponent<PrimevalEssence>(source);

    if (!aperture || !essence)
        return {false, "No aperture or essence."};
    if (worm_slot < 0 || worm_slot >= (int)aperture->worms.size())
        return {false, "No worm in that slot."};

    const auto& worm = aperture->worms[worm_slot];
    const auto& def = *worm.def;

    if (def.range > 0) {
        auto* spos = reg.getComponent<Position>(source);
        auto* tpos = reg.getComponent<Position>(target);
        if (spos && tpos) {
            int dist = chebyshev(*spos, *tpos);
            if (dist > def.range)
                return {
                    false,
                    fmt::format(
                        "Target is out of range (distance {}, {} range {}).",
                        dist,
                        def.name,
                        def.range
                    )};
        }
    }

    if (essence->current < def.essence_cost)
        return {false, "Not enough essence to activate " + def.name + "."};

    essence->current -= def.essence_cost;

    if (def.range == 0) {
        reg.addComponent(source, SelfEffectComponent{def.type, def.effect_value, def.name});
    } else {
        reg.addComponent(
            source, AttackEffectComponent{target, def.effect_value, def.name, def.type}
        );
    }

    return {true, {}};
}
