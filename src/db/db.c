// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <lmdb.h>

#include "../../include/apg/db.h"

MDB_env *
init_db(const char *db_path)
{
    MDB_env *env;
    mdb_env_create(&env);
    mdb_env_set_mapsize(env, 10485760);
    mdb_env_open(env, db_path, 0, 0664);
    return env;
}
