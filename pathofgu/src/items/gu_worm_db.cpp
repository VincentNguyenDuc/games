#include "items/gu_worm_db.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <unordered_map>

void InMemoryGuWormDatabase::add(GuWormDefinition def) {
    auto ptr = std::make_shared<GuWormDefinition>(std::move(def));
    entries_[ptr->name] = ptr;
}

InMemoryGuWormDatabase::InMemoryGuWormDatabase() {
    spdlog::info("Initializing Gu Worm Database");

    // Rank 1
    add({"Strength Gu", 1, GuWormType::Offensive, 5, 8});
    add({"Iron Skin Gu", 1, GuWormType::Defensive, 5, 6});
    add({"Liquor Worm", 1, GuWormType::Recovery, 4, 12});
    add({"Light Gu", 1, GuWormType::Support, 3, 0});
    add({"Stealth Gu", 1, GuWormType::Movement, 4, 0});

    // Rank 2
    add({"Steel Bones Gu", 2, GuWormType::Defensive, 10, 14});
    add({"Lightning Gu", 2, GuWormType::Offensive, 12, 20});
    add({"Jade Skin Gu", 2, GuWormType::Defensive, 8, 10});
    add({"Moonlight Gu", 2, GuWormType::Recovery, 10, 25});
    add({"Wind Footwork Gu", 2, GuWormType::Movement, 8, 0});

    // Rank 3
    add({"Pulling Mountain Gu", 3, GuWormType::Offensive, 18, 38});
    add({"Chainsaw Golden Centipede", 3, GuWormType::Offensive, 20, 45});
    add({"Rock Skin Gu", 3, GuWormType::Defensive, 15, 22});
    add({"Vital Gu", 3, GuWormType::Recovery, 16, 50});

    // Rank 4
    add({"Thunder Stomp Gu", 4, GuWormType::Offensive, 28, 70});
    add({"Fixed Immortal Gu", 4, GuWormType::Support, 25, 0});
    add({"Myriad Self Gu", 4, GuWormType::Defensive, 22, 40});

    // Rank 5
    add({"Spring Autumn Cicada", 5, GuWormType::Support, 40, 0});
    add({"Sword Escape Gu", 5, GuWormType::Movement, 35, 0});
    add({"Boiling Blood Gu", 5, GuWormType::Offensive, 38, 120});
}

std::shared_ptr<GuWormDefinition> InMemoryGuWormDatabase::get(const std::string& name) const {
    auto it = entries_.find(name);
    return it != entries_.end() ? it->second : nullptr;
}

std::vector<std::shared_ptr<GuWormDefinition>> InMemoryGuWormDatabase::all() const {
    std::vector<std::shared_ptr<GuWormDefinition>> result;
    result.reserve(entries_.size());
    for (const auto& [_, def] : entries_)
        result.push_back(def);
    return result;
}

std::vector<std::shared_ptr<GuWormDefinition>> InMemoryGuWormDatabase::by_rank(int rank) const {
    std::vector<std::shared_ptr<GuWormDefinition>> result;
    for (const auto& [_, def] : entries_)
        if (def->rank == rank)
            result.push_back(def);
    return result;
}

std::vector<std::shared_ptr<GuWormDefinition>> InMemoryGuWormDatabase::by_type(GuWormType type
) const {
    std::vector<std::shared_ptr<GuWormDefinition>> result;
    for (const auto& [_, def] : entries_)
        if (def->type == type)
            result.push_back(def);
    return result;
}
