#include "persistence/save_load.hpp"

#include "assets.hpp"
#include "game.hpp"

#include <cstring>
#include <spdlog/spdlog.h>
#include <sqlite3.h>

namespace Persistence {

// ── Schema ────────────────────────────────────────────────────────────────────

static const char* SCHEMA = R"sql(
CREATE TABLE IF NOT EXISTS player (
    id              INTEGER PRIMARY KEY,
    map_id          INTEGER,
    pos_x           INTEGER,
    pos_y           INTEGER,
    level           INTEGER,
    current_xp      INTEGER,
    xp_to_next      INTEGER,
    builder_slots   INTEGER,
    busy_builders   INTEGER,
    visited_forest  INTEGER,
    visited_caves   INTEGER
);
CREATE TABLE IF NOT EXISTS map_state (
    map_id          INTEGER PRIMARY KEY,
    current_map     INTEGER
);
CREATE TABLE IF NOT EXISTS buildings (
    entity_id       INTEGER PRIMARY KEY,
    map_id          INTEGER,
    pos_x           INTEGER,
    pos_y           INTEGER,
    type            INTEGER,
    level           INTEGER,
    hp              INTEGER,
    prod_resource   INTEGER,
    prod_stored     REAL,
    stor_resource   INTEGER,
    stor_current    INTEGER,
    pop_cap         INTEGER,
    has_construction INTEGER,
    con_kind        INTEGER,
    con_time        REAL,
    obs_gold        INTEGER,
    obs_elixir      INTEGER,
    obs_clear_time  REAL,
    obs_remaining   REAL
);
)sql";

// ── Helpers ───────────────────────────────────────────────────────────────────

struct DB {
    sqlite3* handle{nullptr};
    explicit DB(const std::string& path) {
        if (sqlite3_open(path.c_str(), &handle) != SQLITE_OK) {
            spdlog::error("SQLite open failed: {}", sqlite3_errmsg(handle));
            handle = nullptr;
        }
    }
    ~DB() {
        if (handle)
            sqlite3_close(handle);
    }
    bool ok() const { return handle != nullptr; }

    bool exec(const char* sql) {
        char* err = nullptr;
        int rc = sqlite3_exec(handle, sql, nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            spdlog::error("SQLite exec error: {}", err ? err : "unknown");
            sqlite3_free(err);
            return false;
        }
        return true;
    }
};

// ── Save ─────────────────────────────────────────────────────────────────────

void save(const std::string& path, const Game& game) {
    DB db(path);
    if (!db.ok())
        return;

    db.exec("BEGIN;");
    db.exec(SCHEMA);
    db.exec("DELETE FROM player; DELETE FROM map_state; DELETE FROM buildings;");

    const ecse::Engine& ecs = game.ecs();
    const World& world = game.world();
    ecse::Entity player = game.player_entity();

    // ── Player ───────────────────────────────────────────────────────────────
    {
        auto* pl = ecs.registry().getComponent<Player>(player);
        auto* exp = ecs.registry().getComponent<Experience>(player);
        auto* pos = ecs.registry().getComponent<Position>(player);

        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO player VALUES (?,?,?,?,?,?,?,?,?,?,?)";
        sqlite3_prepare_v2(db.handle, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(player));
        sqlite3_bind_int(stmt, 2, pl ? static_cast<int>(pl->current_map_id) : 0);
        sqlite3_bind_int(stmt, 3, pos ? pos->x : 1);
        sqlite3_bind_int(stmt, 4, pos ? pos->y : 1);
        sqlite3_bind_int(stmt, 5, exp ? exp->level : 1);
        sqlite3_bind_int(stmt, 6, exp ? exp->current_xp : 0);
        sqlite3_bind_int(stmt, 7, exp ? exp->xp_to_next : 100);
        sqlite3_bind_int(stmt, 8, pl ? pl->builder_slots : 1);
        sqlite3_bind_int(stmt, 9, pl ? pl->busy_builders : 0);
        sqlite3_bind_int(stmt, 10, pl ? (pl->visited_forest ? 1 : 0) : 0);
        sqlite3_bind_int(stmt, 11, pl ? (pl->visited_caves ? 1 : 0) : 0);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // ── Current map ───────────────────────────────────────────────────────────
    {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db.handle, "INSERT INTO map_state VALUES (0,?)", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, static_cast<int>(world.current_map_id()));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // ── Buildings & obstacles ─────────────────────────────────────────────────
    const char* bsql = "INSERT INTO buildings VALUES "
                       "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

    for (MapId mid = 0; mid < static_cast<MapId>(world.num_maps()); mid++) {
        const Map& m = world.get_map(mid);
        for (ecse::Entity e : m.entities) {
            auto* pos = ecs.registry().getComponent<Position>(e);
            auto* bld = ecs.registry().getComponent<Building>(e);
            auto* lv = ecs.registry().getComponent<Level>(e);
            auto* obs = ecs.registry().getComponent<Obstacle>(e);
            if (!pos)
                continue;

            auto* prd = ecs.registry().getComponent<ResourceProducer>(e);
            auto* sto = ecs.registry().getComponent<ResourceStorage>(e);
            auto* pop = ecs.registry().getComponent<PopulationProvider>(e);
            auto* con = ecs.registry().getComponent<Construction>(e);

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.handle, bsql, -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, static_cast<int>(e));
            sqlite3_bind_int(stmt, 2, static_cast<int>(mid));
            sqlite3_bind_int(stmt, 3, pos->x);
            sqlite3_bind_int(stmt, 4, pos->y);
            sqlite3_bind_int(stmt, 5, bld ? static_cast<int>(bld->type) : -1);
            sqlite3_bind_int(stmt, 6, lv ? lv->level : 1);
            sqlite3_bind_int(stmt, 7, bld ? bld->hp : 0);
            sqlite3_bind_int(stmt, 8, prd ? static_cast<int>(prd->resource) : -1);
            sqlite3_bind_double(stmt, 9, prd ? static_cast<double>(prd->stored) : 0.0);
            sqlite3_bind_int(stmt, 10, sto ? static_cast<int>(sto->resource) : -1);
            sqlite3_bind_int(stmt, 11, sto ? sto->current : 0);
            sqlite3_bind_int(stmt, 12, pop ? pop->capacity : 0);
            sqlite3_bind_int(stmt, 13, con ? 1 : 0);
            sqlite3_bind_int(stmt, 14, con ? static_cast<int>(con->kind) : 0);
            sqlite3_bind_double(stmt, 15, con ? static_cast<double>(con->time_remaining_s) : 0.0);
            sqlite3_bind_int(stmt, 16, obs ? obs->gold_reward : 0);
            sqlite3_bind_int(stmt, 17, obs ? obs->elixir_reward : 0);
            sqlite3_bind_double(stmt, 18, obs ? static_cast<double>(obs->clear_time_s) : 0.0);
            sqlite3_bind_double(stmt, 19, obs ? static_cast<double>(obs->time_remaining_s) : 0.0);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    db.exec("COMMIT;");
}

// ── Load ─────────────────────────────────────────────────────────────────────

bool load(const std::string& path, Game& game) {
    DB db(path);
    if (!db.ok())
        return false;

    // Check tables exist
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(
                db.handle,
                "SELECT name FROM sqlite_master WHERE type='table' AND name='player'",
                -1,
                &stmt,
                nullptr
            ) != SQLITE_OK)
            return false;
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_ROW)
            return false;
    }

    ecse::Engine& ecs = game.ecs();
    World& world = game.world();
    ecse::Entity player = game.player_entity();

    // ── Player ───────────────────────────────────────────────────────────────
    {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db.handle, "SELECT * FROM player LIMIT 1", -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            MapId mid = static_cast<MapId>(sqlite3_column_int(stmt, 1));
            int px = sqlite3_column_int(stmt, 2);
            int py = sqlite3_column_int(stmt, 3);
            int lv = sqlite3_column_int(stmt, 4);
            int xp = sqlite3_column_int(stmt, 5);
            int xpn = sqlite3_column_int(stmt, 6);
            int bslots = sqlite3_column_int(stmt, 7);
            int bbusy = sqlite3_column_int(stmt, 8);
            bool vforest = sqlite3_column_int(stmt, 9) != 0;
            bool vcaves = sqlite3_column_int(stmt, 10) != 0;

            if (auto* p = ecs.registry().getComponent<Player>(player)) {
                p->current_map_id = mid;
                p->builder_slots = bslots;
                p->busy_builders = bbusy;
                p->visited_forest = vforest;
                p->visited_caves = vcaves;
            }
            if (auto* e = ecs.registry().getComponent<Experience>(player)) {
                e->level = lv;
                e->current_xp = xp;
                e->xp_to_next = xpn;
            }
            if (auto* p = ecs.registry().getComponent<Position>(player)) {
                p->x = px;
                p->y = py;
            }

            world.travel_to(mid);
        }
        sqlite3_finalize(stmt);
    }

    // ── Buildings & obstacles ─────────────────────────────────────────────────
    // Clear all existing map entities first (they were spawned in constructor)
    for (MapId mid = 0; mid < static_cast<MapId>(world.num_maps()); mid++) {
        Map& m = world.get_map(mid);
        for (ecse::Entity e : m.entities)
            ecs.entities().destroyEntity(e);
        m.entities.clear();
        m.entity_at.clear();
    }

    {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db.handle, "SELECT * FROM buildings", -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            MapId mid = static_cast<MapId>(sqlite3_column_int(stmt, 1));
            int px = sqlite3_column_int(stmt, 2);
            int py = sqlite3_column_int(stmt, 3);
            int btype = sqlite3_column_int(stmt, 4);
            int lv = sqlite3_column_int(stmt, 5);
            int hp = sqlite3_column_int(stmt, 6);
            int pres = sqlite3_column_int(stmt, 7);
            double pst = sqlite3_column_double(stmt, 8);
            int sres = sqlite3_column_int(stmt, 9);
            int scur = sqlite3_column_int(stmt, 10);
            int popc = sqlite3_column_int(stmt, 11);
            bool hasc = sqlite3_column_int(stmt, 12) != 0;
            int ckind = sqlite3_column_int(stmt, 13);
            double ctime = sqlite3_column_double(stmt, 14);
            int ogold = sqlite3_column_int(stmt, 15);
            int oelix = sqlite3_column_int(stmt, 16);
            double oct = sqlite3_column_double(stmt, 17);
            double ort = sqlite3_column_double(stmt, 18);

            if (!world.map_exists(mid))
                continue;

            if (btype < 0) {
                // Obstacle
                ecse::Entity e = ecs.entities().createEntity();
                ecs.registry().addComponent<Position>(e, {px, py});
                ecs.registry().addComponent<MapLocation>(e, {mid});
                ecs.registry().addComponent<Obstacle>(
                    e, {ogold, oelix, static_cast<float>(oct), static_cast<float>(ort)}
                );
                world.get_map(mid).place_entity(px, py, e);
            } else {
                // Building — use spawn_building to recreate components
                auto type = static_cast<BuildingType>(btype);
                // We can't call spawn_building directly (private), so recreate manually.
                // We replicate the logic here for loaded state.
                ecse::Entity e = ecs.entities().createEntity();
                const auto& d = Assets::def(type);
                const auto& stat = d.stats[lv - 1];

                ecs.registry().addComponent<Position>(e, {px, py});
                ecs.registry().addComponent<MapLocation>(e, {mid});
                ecs.registry().addComponent<Name>(e, {std::string(d.name)});
                ecs.registry().addComponent<Building>(e, {type, 1, stat.hp, hp});
                ecs.registry().addComponent<Level>(e, {lv, d.max_level});

                if (pres >= 0) {
                    ecs.registry().addComponent<ResourceProducer>(
                        e,
                        {static_cast<ResourceType>(pres),
                         stat.rate_per_hour,
                         static_cast<float>(pst),
                         stat.prod_cap}
                    );
                }
                if (sres >= 0) {
                    ecs.registry().addComponent<ResourceStorage>(
                        e, {static_cast<ResourceType>(sres), scur, stat.storage_cap}
                    );
                }
                if (popc > 0) {
                    ecs.registry().addComponent<PopulationProvider>(e, {popc});
                }
                if (type == BuildingType::BuilderHut) {
                    ecs.registry().addComponent<Builder>(e, {false, 0});
                }
                if (lv < d.max_level) {
                    const auto& next = d.stats[lv];
                    ecs.registry().addComponent<UpgradeCost>(e, {next.gold_cost, next.elixir_cost});
                }
                if (hasc) {
                    ecs.registry().addComponent<Construction>(
                        e, {static_cast<ConstructionKind>(ckind), 0, static_cast<float>(ctime)}
                    );
                }

                world.get_map(mid).place_entity(px, py, e);
            }
        }
        sqlite3_finalize(stmt);
    }

    return true;
}

} // namespace Persistence
