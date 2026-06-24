# Path of Gu

A turn-based roguelike dungeon crawler set in a cultivation fantasy world. You play as Fang Yuan, a Gu Master navigating a 7-level Grotto-Heaven, collecting and deploying Gu worms to defeat enemies and reach the exit.

## How to Play

Move with arrow keys or type commands:

| Input | Action |
|---|---|
| Arrow keys | Move north/south/east/west |
| `attack <slot>` | Activate an offensive worm (slot 0–2) |
| `heal <slot>` | Activate a recovery worm on yourself |
| `pickup <index>` | Pick up a dropped worm from the ground |
| `drop <index>` | Drop a worm from your aperture |
| `skip` / Space | Pass your turn (restores essence out of combat) |
| `quit` | Exit the game |

## Win & Loss Conditions

- **Win** — reach the final map (map 7) and defeat the Immortal's Guardian.
- **Loss (death)** — your HP drops to 0.
- **Loss (collapse)** — your Primeval Essence hits 0 for 3 consecutive turns; your aperture collapses.

## Game World

The dungeon is a linear chain of 7 maps, each a 10×10 grid. Maps connect via door cells (◆). Enemies spawn at random positions on their assigned map.

```
[Map 1] ─ [Map 2] ─ [Map 3] ─ [Map 4] ─ [Map 5 Cache] ─ [Map 6] ─ [Map 7 Exit]
```

Map 5 is a safe cache room containing two free Gu worms (Moonlight Gu, Steel Bones Gu) with no enemies.

## Player Stats

| Stat | Starting Value | Notes |
|---|---|---|
| HP | 100 / 100 | Reaches 0 → instant loss |
| Primeval Essence | 60 / 60 | Resource for activating worms |
| Cultivation Rank | 1 | Determines aperture capacity (rank × 3 slots) |
| Aperture capacity | 3 slots | Holds equipped Gu worms |

**Essence regeneration:**
- Successful move: +20% of max (min 1), depleted counter resets
- Skip (out of combat): +20% of max
- Depleted for 3 turns in a row → aperture collapses → defeat

## Gu Worms

Worms are the core resource. Each has a type, essence cost, effect value, and range:

| Type | Effect | Range |
|---|---|---|
| Offensive | Deals damage to target | ≥ 1 |
| Defensive | Applies armor/damage reduction | ≥ 1 |
| Recovery | Heals HP | 0 (self only) |
| Support | Drains target's essence | ≥ 1 |

**Range** is Chebyshev distance — 1 means adjacent (including diagonals), 0 means self-targeting only.

### Notable Worms

| Worm | Type | Cost | Effect | Range |
|---|---|---|---|---|
| Strength Gu | Offensive | — | Raw damage | 1 |
| Iron Skin Gu | Defensive | — | Armor | 1 |
| Lightning Gu | Offensive | — | High damage | — |
| Vital Gu | Recovery | — | Heals HP | 0 |
| Thunder Stomp Gu | Offensive | — | AoE-style | — |
| Moonlight Gu | — | — | Cache room reward | — |
| Fixed Immortal Gu | — | — | Boss drop | — |

## Enemies

### Behavior Types

| Type | Style | Essence | Attack Range |
|---|---|---|---|
| Wild | Attacks on sight, no tactics | 30 | 1 |
| Schemer | Uses worms tactically; prefers defense when hurt | 50 | 3 |
| Guardian | 30% chance of double-damage power strike | 80 | 2 |

### Enemy Roster

| Name | Map | HP | Behavior | Notable Drops |
|---|---|---|---|---|
| Wild Strength Gu | 1, 2 | 18 | Wild | Strength Gu (80%) |
| Wild Iron Skin Worm | 2 | 22 | Wild | Iron Skin Gu (60%) |
| Demonic Gu Master - Wei | 3 | 45 | Schemer | Lightning Gu (70%), Iron Skin Gu (50%) |
| Demonic Gu Master - Liu | 4 | 50 | Schemer | Jade Skin Gu (70%), Vital Gu (40%) |
| Corrupted Worm Construct | 4 | 28 | Wild | Strength Gu (50%) |
| Iron Guardian Construct | 6 | 65 | Guardian | Rock Skin Gu (100%), Thunder Stomp Gu (50%) |
| Immortal's Guardian | 7 | 120 | Guardian | Boiling Blood Gu (100%), Fixed Immortal Gu (60%) |

## Turn Structure

Every time the player takes an action (attack, heal, or skip), a full tick runs:

1. **AI phase** — each enemy on the current map decides independently (parallel `std::async`):
   - If out of attack range → move toward player
   - If in range → pick best worm or raw attack based on HP and behavior type
2. **Movement phase** — all stamped `MoveIntent` components are resolved (collision, door transitions)
3. **Self-effect phase** — heals and buffs are applied
4. **Attack phase** — damage and essence drain effects are applied
5. **Cleanup** — dead entities are removed; loot is rolled and dropped

Movement (arrow keys) does **not** trigger a full tick — only the player moves.

## ECS Components

| Component | Purpose |
|---|---|
| `Health` | HP pool |
| `Position` | Map ID + x/y coordinates |
| `Stats` | base_attack, base_defense, attack_range |
| `PrimevalEssence` | Essence pool + depletion counter |
| `CultivationRank` | Rank + refinement points |
| `Aperture` | Equipped worm slots |
| `AIBehavior` | Behavior type (Wild / Schemer / Guardian) |
| `Name` | Display name |
| `Loot` | Drop table |
| `MoveIntentComponent` | *Transient* — pending movement this tick |
| `AttackEffectComponent` | *Transient* — pending attack effect this tick |
| `SelfEffectComponent` | *Transient* — pending self-heal/buff this tick |
