#pragma once

enum class BehaviorType {
    Wild,     // attacks on sight, no strategy
    Schemer,  // rival Gu Master — uses Gu worms tactically
    Guardian, // boss — fixed attack patterns, high stats
};

struct AIBehavior {
    BehaviorType type;
};
