#include "actions.hpp"

#include "assets.hpp"
#include "game.hpp"

#include <algorithm>
#include <fmt/format.h>

Actions::Actions(Game& g)
    : g_(g) {}

// ─── Player actions ───────────────────────────────────────────────────────────

void Actions::try_move(int dx, int dy) {
    auto* pos = g_.ecs_.registry().getComponent<Position>(g_.player_);
    auto* pl = g_.ecs_.registry().getComponent<Player>(g_.player_);
    if (!pos || !pl)
        return;

    int nx = pos->x + dx;
    int ny = pos->y + dy;
    Map& m = g_.world_.get_map(pl->current_map_id);

    if (!m.in_bounds(nx, ny))
        return;

    int cell = m.grid[ny][nx];

    if (cell == Cell::Gate) {
        auto it = m.gates.find(m.cell_key(nx, ny));
        if (it == m.gates.end())
            return;
        const Gate& gate = it->second;
        Map& target = g_.world_.get_map(gate.target_map);

        auto* exp = g_.ecs_.registry().getComponent<Experience>(g_.player_);
        if (exp && exp->level < target.required_player_level) {
            g_.push_msg(
                fmt::format("Need level {} to enter {}.", target.required_player_level, target.name)
            );
            return;
        }

        pl->current_map_id = gate.target_map;
        pos->x = gate.target_x;
        pos->y = gate.target_y;

        if (gate.target_map == 1 && !pl->visited_forest) {
            pl->visited_forest = true;
            g_.grant_xp(50);
            g_.push_msg(fmt::format("Entered {}! +50 XP", target.name));
        } else if (gate.target_map == 2 && !pl->visited_caves) {
            pl->visited_caves = true;
            g_.grant_xp(100);
            g_.push_msg(fmt::format("Entered {}! +100 XP", target.name));
        } else {
            g_.push_msg(fmt::format("Entered {}.", target.name));
        }
        g_.world_.travel_to(gate.target_map);
        g_.selected_ = 0;
        return;
    }

    if (m.walkable(nx, ny)) {
        pos->x = nx;
        pos->y = ny;
        g_.selected_ = 0;
    }
}

void Actions::try_interact() {
    auto* pos = g_.ecs_.registry().getComponent<Position>(g_.player_);
    auto* pl = g_.ecs_.registry().getComponent<Player>(g_.player_);
    if (!pos || !pl)
        return;

    Map& m = g_.world_.current_map();
    ecse::Entity e = m.get_entity(pos->x, pos->y);

    if (!e) {
        for (auto [dx, dy] :
             std::initializer_list<std::pair<int, int>>{{0, -1}, {0, 1}, {-1, 0}, {1, 0}}) {
            ecse::Entity adj = m.get_entity(pos->x + dx, pos->y + dy);
            if (adj) {
                e = adj;
                break;
            }
        }
    }

    if (!e) {
        g_.selected_ = 0;
        return;
    }

    g_.selected_ = e;

    if (auto* prod = g_.ecs_.registry().getComponent<ResourceProducer>(e)) {
        if (prod->stored >= 1.f) {
            do_collect(e);
            return;
        }
    }

    if (auto* obs = g_.ecs_.registry().getComponent<Obstacle>(e)) {
        if (obs->time_remaining_s <= 0.f)
            do_clear_obstacle(e);
        return;
    }
}

void Actions::do_collect(ecse::Entity e) {
    auto* prod = g_.ecs_.registry().getComponent<ResourceProducer>(e);
    if (!prod || prod->stored < 1.f)
        return;

    int amount = static_cast<int>(prod->stored);
    if (prod->resource == ResourceType::Gold) {
        int cap = g_.total_gold_cap() - g_.total_gold();
        amount = std::min(amount, cap);
        g_.add_resources(amount, 0);
    } else {
        int cap = g_.total_elixir_cap() - g_.total_elixir();
        amount = std::min(amount, cap);
        g_.add_resources(0, amount);
    }
    prod->stored -= static_cast<float>(amount);
    g_.grant_xp(amount / 50);
    g_.push_msg(fmt::format(
        "Collected {} {}.", amount, prod->resource == ResourceType::Gold ? "gold" : "elixir"
    ));
}

void Actions::try_upgrade() {
    if (!g_.selected_)
        return;
    ecse::Entity e = g_.selected_;

    auto* lv = g_.ecs_.registry().getComponent<Level>(e);
    auto* cost = g_.ecs_.registry().getComponent<UpgradeCost>(e);
    auto* bld = g_.ecs_.registry().getComponent<Building>(e);
    if (!lv || !cost || !bld) {
        g_.push_msg("Can't upgrade this.");
        return;
    }
    if (g_.ecs_.registry().getComponent<Construction>(e)) {
        g_.push_msg("Already under construction.");
        return;
    }
    if (g_.free_builders() <= 0) {
        g_.push_msg("No free builders!");
        return;
    }
    if (!g_.spend_resources(cost->gold, cost->elixir)) {
        g_.push_msg("Not enough resources.");
        return;
    }

    const auto& d = Assets::def(bld->type);
    float time_s = static_cast<float>(d.stats[lv->level].build_time_s);
    start_construction(e, ConstructionKind::Upgrade, time_s);
    g_.push_msg(
        fmt::format("Upgrading {} to lv{}… ({})", d.name, lv->level + 1, Game::fmt_time(time_s))
    );
}

void Actions::try_clear() {
    if (!g_.selected_)
        return;
    ecse::Entity e = g_.selected_;
    auto* obs = g_.ecs_.registry().getComponent<Obstacle>(e);
    if (!obs) {
        g_.push_msg("Nothing to clear.");
        return;
    }
    if (obs->time_remaining_s > 0.f) {
        g_.push_msg("Already clearing.");
        return;
    }
    if (g_.free_builders() <= 0) {
        g_.push_msg("No free builders!");
        return;
    }
    do_clear_obstacle(e);
}

void Actions::do_clear_obstacle(ecse::Entity e) {
    auto* obs = g_.ecs_.registry().getComponent<Obstacle>(e);
    if (!obs)
        return;
    obs->time_remaining_s = obs->clear_time_s;
    auto* p = g_.ecs_.registry().getComponent<Player>(g_.player_);
    if (p)
        p->busy_builders++;
    g_.push_msg(fmt::format("Clearing obstacle… ({})", Game::fmt_time(obs->clear_time_s)));
}

void Actions::finish_obstacle(ecse::Entity e) {
    auto* obs = g_.ecs_.registry().getComponent<Obstacle>(e);
    auto* pos = g_.ecs_.registry().getComponent<Position>(e);
    auto* ml = g_.ecs_.registry().getComponent<MapLocation>(e);
    if (!obs || !pos || !ml)
        return;

    g_.add_resources(obs->gold_reward, obs->elixir_reward);
    g_.grant_xp(25);
    g_.push_msg(
        fmt::format("Obstacle cleared! +{}G +{}E +25 XP", obs->gold_reward, obs->elixir_reward)
    );

    g_.world_.get_map(ml->map_id).remove_entity_at(pos->x, pos->y);
    if (g_.selected_ == e)
        g_.selected_ = 0;

    auto* p = g_.ecs_.registry().getComponent<Player>(g_.player_);
    if (p && p->busy_builders > 0)
        p->busy_builders--;

    g_.ecs_.entities().destroyEntity(e);
    g_.ecs_.registry().removeComponent<Position>(e);
    g_.ecs_.registry().removeComponent<MapLocation>(e);
    g_.ecs_.registry().removeComponent<Obstacle>(e);
}

void Actions::start_construction(ecse::Entity e, ConstructionKind kind, float time_s) {
    g_.ecs_.registry().addComponent<Construction>(e, {kind, 0, time_s});
    auto* p = g_.ecs_.registry().getComponent<Player>(g_.player_);
    if (p)
        p->busy_builders++;
    g_.ecs_.registry().removeComponent<UpgradeCost>(e);
}

void Actions::finish_construction(ecse::Entity e, bool is_upgrade) {
    auto* bld = g_.ecs_.registry().getComponent<Building>(e);
    auto* lv = g_.ecs_.registry().getComponent<Level>(e);
    if (!bld || !lv)
        return;

    if (is_upgrade) {
        lv->level++;
        const auto& d = Assets::def(bld->type);
        const auto& stat = d.stats[lv->level - 1];
        bld->max_hp = stat.hp;
        bld->hp = stat.hp;

        if (auto* prod = g_.ecs_.registry().getComponent<ResourceProducer>(e)) {
            prod->rate_per_hour = stat.rate_per_hour;
            prod->capacity = stat.prod_cap;
        }
        if (auto* stor = g_.ecs_.registry().getComponent<ResourceStorage>(e)) {
            stor->capacity = stat.storage_cap;
        }
        if (auto* pop = g_.ecs_.registry().getComponent<PopulationProvider>(e)) {
            pop->capacity = stat.pop_cap;
        }

        if (lv->level < d.max_level) {
            const auto& next = d.stats[lv->level];
            g_.ecs_.registry().addComponent<UpgradeCost>(e, {next.gold_cost, next.elixir_cost});
        }

        int xp_award = 30 * lv->level;
        g_.grant_xp(xp_award);
        g_.push_msg(fmt::format("{} upgraded to lv{}! +{} XP", d.name, lv->level, xp_award));
    } else {
        g_.grant_xp(20);
        auto* nm = g_.ecs_.registry().getComponent<Name>(e);
        g_.push_msg(fmt::format("{} built! +20 XP", nm ? nm->value : "Building"));
    }

    auto* p = g_.ecs_.registry().getComponent<Player>(g_.player_);
    if (p && p->busy_builders > 0)
        p->busy_builders--;
}

void Actions::enter_build_mode() {
    g_.mode_ = Game::Mode::BuildSelect;
    g_.build_menu_idx_ = 0;
    g_.push_msg("Build mode: +/- to pick type, Enter to place, Esc to cancel.");
}

void Actions::confirm_build() {
    auto* pos = g_.ecs_.registry().getComponent<Position>(g_.player_);
    auto* pl = g_.ecs_.registry().getComponent<Player>(g_.player_);
    if (!pos || !pl)
        return;

    Map& m = g_.world_.current_map();
    if (!m.in_bounds(pos->x, pos->y) || m.has_entity(pos->x, pos->y) ||
        m.grid[pos->y][pos->x] != Cell::Empty) {
        g_.push_msg("Can't build here.");
        return;
    }
    if (g_.free_builders() <= 0) {
        g_.push_msg("No free builders!");
        return;
    }

    const auto& d = Assets::def(g_.build_cursor_type_);
    const auto& stat = d.stats[0];

    if (!g_.spend_resources(stat.gold_cost, stat.elixir_cost)) {
        g_.push_msg("Not enough resources.");
        return;
    }

    ecse::Entity e = spawn_building(pl->current_map_id, pos->x, pos->y, g_.build_cursor_type_, 1);
    start_construction(e, ConstructionKind::Build, static_cast<float>(stat.build_time_s));

    g_.mode_ = Game::Mode::Normal;
    g_.selected_ = e;
    g_.push_msg(fmt::format(
        "Building {}… ({})", d.name, Game::fmt_time(static_cast<float>(stat.build_time_s))
    ));
}

void Actions::cancel_build() {
    g_.mode_ = Game::Mode::Normal;
    g_.push_msg("Build cancelled.");
}

void Actions::on_level_up(int new_level) {
    auto* pl = g_.ecs_.registry().getComponent<Player>(g_.player_);
    if (pl)
        pl->builder_slots = Assets::builder_slots_for_level(new_level);

    std::string bonus;
    if (new_level == 3)
        bonus = " Ancient Forest unlocked!";
    else if (new_level == 5)
        bonus = " +1 builder slot!";
    else if (new_level == 7)
        bonus = " Crystal Caves unlocked!";
    else if (new_level == 10)
        bonus = " +1 builder slot!";

    g_.push_msg(fmt::format("Level up! Now level {}.{}", new_level, bonus));
}
