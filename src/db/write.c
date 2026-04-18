// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <lmdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../../include/apg/db.h"
#include "../../include/apg/package.h"
#include "../../include/apg/json.h"

bool
add_package(struct package *pkg, MDB_env *env)
{
    MDB_dbi dbi;
    MDB_val key, data;
    MDB_txn *txn;

    if (mdb_txn_begin(env, NULL, 0, &txn) != MDB_SUCCESS)
        return false;
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        return false;
    }

    char *s_value = package_to_json(pkg);
    if (!s_value) {
        mdb_txn_abort(txn);
        return false;
    }

    key.mv_size = strlen(pkg->meta->name);
    key.mv_data = pkg->meta->name;
    data.mv_size = strlen(s_value);
    data.mv_data = s_value;

    bool ok = mdb_put(txn, dbi, &key, &data, 0) == MDB_SUCCESS;
    free(s_value);

    if (ok)
        mdb_txn_commit(txn);
    else
        mdb_txn_abort(txn);

    mdb_dbi_close(env, dbi);
    return ok;
}

bool
remove_package(char *pkg_name, MDB_env *env)
{
    MDB_dbi dbi;
    MDB_val key;
    MDB_txn *txn;

    if (mdb_txn_begin(env, NULL, 0, &txn) != MDB_SUCCESS)
        return false;
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        return false;
    }

    key.mv_size = strlen(pkg_name);
    key.mv_data = pkg_name;

    bool ok = mdb_del(txn, dbi, &key, NULL) == MDB_SUCCESS;

    if (ok)
        mdb_txn_commit(txn);
    else
        mdb_txn_abort(txn);

    mdb_dbi_close(env, dbi);
    return ok;
}
