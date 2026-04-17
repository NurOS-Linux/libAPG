// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <lmdb.h>
#include <stdbool.h>

#include "../include/apg/db.h"
#include "../include/apg/package.h"
#include "../include/apg/json.h"
#include <string.h>


MDB_env *
init_db_env(const char *db_path)
{
    MDB_env *env;
    mdb_env_create(&env);
    mdb_env_set_mapsize(env, 10485760);
    mdb_env_open(env, db_path, 0, 0664);
    return env;
}

bool
add_package(struct package *pkg, MDB_env *env)
{
    MDB_dbi dbi;
    MDB_val key, data;
    MDB_txn *txn;

    mdb_txn_begin(env, NULL, 0, &txn);
    mdb_dbi_open(txn, NULL, 0, &dbi);
    mdb_txn_commit(txn);

    // write
    char *s_key = pkg->meta->name;
    char *s_value = package_to_json(pkg);
    key.mv_size = strlen(s_key);
    key.mv_data = s_key;
    data.mv_size = strlen(s_value);
    data.mv_data = s_value;

    mdb_txn_begin(env, NULL, 0, &txn);
    mdb_put(txn, dbi, &key, &data, 0);
    mdb_txn_commit(txn);

    mdb_dbi_close(env, dbi);

    return true;
}

