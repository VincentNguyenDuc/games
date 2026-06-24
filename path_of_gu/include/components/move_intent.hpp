#pragma once

// Stamped on an entity to signal desired movement this turn.
// Processed and removed by resolve_moves().
struct MoveIntentComponent {
    int dx = 0;
    int dy = 0;
    bool allow_slide = false; // if true, try axis-aligned fallback when diagonal is blocked
};
