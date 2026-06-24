#include "systems/effect_system.hpp"

#include "components/attack_effect.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/primeval_essence.hpp"
#include "components/self_effect.hpp"
#include "components/stats.hpp"

#include <fmt/format.h>

SelfEffectTickSystem::SelfEffectTickSystem() {
    reads = {
        ComponentType(typeid(SelfEffectComponent)),
        ComponentType(typeid(Health)),
        ComponentType(typeid(Name))};
    writes = {ComponentType(typeid(Health)), ComponentType(typeid(SelfEffectComponent))};
}

std::string SelfEffectTickSystem::resolve(EntityComponentRegistry& reg) {
    std::string out;

    for (Entity e : reg.view<SelfEffectComponent>()) {
        auto* eff = reg.getComponent<SelfEffectComponent>(e);
        auto* hp = reg.getComponent<Health>(e);
        auto* name = reg.getComponent<Name>(e);
        std::string ename = name ? name->value : "Entity";

        switch (eff->worm_type) {
        case GuWormType::Recovery:
            if (hp) {
                int healed = std::min(eff->effect_value, hp->max_hp - hp->hp);
                hp->hp += healed;
                out += fmt::format(
                    "{} activates {}! Restores {} HP ({}/{}).\n",
                    ename,
                    eff->worm_name,
                    healed,
                    hp->hp,
                    hp->max_hp
                );
            }
            break;
        case GuWormType::Support:
        case GuWormType::Movement:
            out += fmt::format("{} activates {}.\n", ename, eff->worm_name);
            break;
        default:
            break;
        }

        reg.removeComponent<SelfEffectComponent>(e);
    }

    return out;
}

void SelfEffectTickSystem::update(EntityComponentRegistry& reg, CommandBuffer&) {
    output = resolve(reg);
}

AttackEffectTickSystem::AttackEffectTickSystem() {
    reads = {
        ComponentType(typeid(AttackEffectComponent)),
        ComponentType(typeid(Health)),
        ComponentType(typeid(Stats)),
        ComponentType(typeid(PrimevalEssence)),
        ComponentType(typeid(Name)),
    };
    writes = {
        ComponentType(typeid(Health)),
        ComponentType(typeid(PrimevalEssence)),
        ComponentType(typeid(AttackEffectComponent)),
    };
}

std::string AttackEffectTickSystem::resolve(EntityComponentRegistry& reg) {
    std::string out;

    for (Entity attacker : reg.view<AttackEffectComponent>()) {
        auto* eff = reg.getComponent<AttackEffectComponent>(attacker);
        auto* src_name = reg.getComponent<Name>(attacker);
        auto* tgt_hp = reg.getComponent<Health>(eff->target);
        auto* tgt_stats = reg.getComponent<Stats>(eff->target);
        auto* tgt_ess = reg.getComponent<PrimevalEssence>(eff->target);
        auto* tgt_name = reg.getComponent<Name>(eff->target);

        std::string sname = src_name ? src_name->value : "Entity";
        std::string tname = tgt_name ? tgt_name->value : "Entity";

        switch (eff->worm_type) {
        case GuWormType::Offensive:
        case GuWormType::Defensive: {
            if (!tgt_hp)
                break;
            int defense = tgt_stats ? std::max(0, tgt_stats->base_defense) : 0;
            int damage = std::max(1, eff->effect_value - defense);
            tgt_hp->hp -= damage;

            if (eff->worm_name.empty()) {
                out += fmt::format("{} strikes {} for {} damage!\n", sname, tname, damage);
            } else {
                out += fmt::format(
                    "{} activates {}! Deals {} damage to {}{}.\n",
                    sname,
                    eff->worm_name,
                    damage,
                    tname,
                    tgt_hp->hp <= 0 ? " — defeated!" : ""
                );
            }
            break;
        }
        case GuWormType::Support: {
            if (!tgt_ess)
                break;
            tgt_ess->current = std::max(0, tgt_ess->current - eff->effect_value);
            out += fmt::format(
                "{} disrupts {}'s aperture, draining {} essence!\n", sname, tname, eff->effect_value
            );
            break;
        }
        default:
            break;
        }

        reg.removeComponent<AttackEffectComponent>(attacker);
    }

    return out;
}

void AttackEffectTickSystem::update(EntityComponentRegistry& reg, CommandBuffer&) {
    output = resolve(reg);
}
