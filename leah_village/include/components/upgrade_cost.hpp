#pragma once

// Cost to advance this building to its next level.
// Attached to any upgradeable building; removed when an upgrade begins.
struct UpgradeCost {
    int gold;
    int elixir;
};
