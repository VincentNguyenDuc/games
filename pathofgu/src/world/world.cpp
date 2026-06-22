#include "world/world.hpp"

#include <algorithm>
#include <stdexcept>

// Dungeon layout:
//
//  [0] Collapsed Archway (entrance)
//       | south
//  [1] Spirit Stone Passage
//       | south              | east
//  [2] Worm Hive Chamber    [5] Forgotten Cache (dead end, loot)
//       | south
//  [3] Bloodstained Hall
//       | south              | west
//  [4] Ancient Refinement   [6] Sealed Side Chamber (dead end, loot)
//       | south
//  [7] Guardian's Sanctum (boss / exit)

World::World() {
    auto entrance = make_room(
        "Collapsed Archway",
        "You awaken amid shattered jade pillars. The air reeks of decayed Gu worms."
    );
    auto passage = make_room(
        "Spirit Stone Passage",
        "Faintly glowing spirit stones line the walls. Several have already gone dark."
    );
    auto hive = make_room(
        "Worm Hive Chamber", "The floor writhes. Dozens of rank-1 Gu worms nest in the cracks."
    );
    auto hall = make_room(
        "Bloodstained Hall",
        "Dried blood traces old battles between Gu Masters who came before you."
    );
    auto refinery = make_room(
        "Ancient Refinement Chamber",
        "Rusted refinement cauldrons line the walls. Some worms still circle the vats."
    );
    auto cache = make_room(
        "Forgotten Cache",
        "A cramped alcove sealed by a collapsed pillar. Worm casings litter the floor."
    );
    auto sealed = make_room(
        "Sealed Side Chamber",
        "A reinforced door left ajar. The previous occupant did not leave voluntarily."
    );
    auto sanctum = make_room(
        "Guardian's Sanctum",
        "The Immortal's last construct stands here, waiting. The exit seal glows ahead.",
        true
    );

    connect(entrance, "south", passage);
    connect(passage, "south", hive);
    connect(hive, "south", hall);
    connect(hive, "east", cache);
    connect(hall, "south", refinery);
    connect(refinery, "south", sanctum);
    connect(refinery, "west", sealed);
}

RoomId World::make_room(std::string name, std::string description, bool is_exit) {
    RoomId id = next_id_++;
    auto room = std::make_unique<Room>();
    room->id = id;
    room->name = std::move(name);
    room->description = std::move(description);
    room->is_exit = is_exit;
    rooms_[id] = std::move(room);
    return id;
}

void World::connect(RoomId a, const std::string& dir, RoomId b) {
    static const std::unordered_map<std::string, std::string> opposites = {
        {"north", "south"},
        {"south", "north"},
        {"east", "west"},
        {"west", "east"},
    };
    rooms_.at(a)->exits[dir] = b;
    rooms_.at(b)->exits[opposites.at(dir)] = a;
}

Room* World::get_room(RoomId id) {
    auto it = rooms_.find(id);
    return it != rooms_.end() ? it->second.get() : nullptr;
}

void World::add_entity(RoomId room_id, Entity entity) {
    rooms_.at(room_id)->entities.push_back(entity);
}

void World::remove_entity(RoomId room_id, Entity entity) {
    auto& entities = rooms_.at(room_id)->entities;
    entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
}

void World::move_entity(Entity entity, RoomId from, RoomId to) {
    remove_entity(from, entity);
    add_entity(to, entity);
}
