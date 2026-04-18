// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <lmdb.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/apg/db.h"
#include "../../include/apg/package.h"
#include "../../include/apg/json.h"

struct package *
get_package_by_name(char *name, MDB_env *env)
{
    MDB_dbi dbi;
    MDB_val key, data;
    MDB_txn *txn;

    if (mdb_txn_begin(env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return NULL;
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        return NULL;
    }

    key.mv_size = strlen(name);
    key.mv_data = name;

    if (mdb_get(txn, dbi, &key, &data) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        mdb_dbi_close(env, dbi);
        return NULL;
    }

    struct package *pkg = package_new();
    if (pkg) {
        package_metadata_free(pkg->meta);
        pkg->meta = package_metadata_from_json(data.mv_data, data.mv_size);
        if (!pkg->meta) {
            package_free(pkg);
            pkg = NULL;
        }
    }

    mdb_txn_abort(txn);
    mdb_dbi_close(env, dbi);
    return pkg;
}

struct package **
get_all_packages(MDB_env *env, int *count)
{
    *count = 0;

    MDB_dbi dbi;
    MDB_txn *txn;

    if (mdb_txn_begin(env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return NULL;
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        return NULL;
    }

    MDB_cursor *cursor;
    if (mdb_cursor_open(txn, dbi, &cursor) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        mdb_dbi_close(env, dbi);
        return NULL;
    }

    int capacity = 16;
    struct package **list = malloc(capacity * sizeof(struct package *));
    if (!list) {
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        mdb_dbi_close(env, dbi);
        return NULL;
    }

    MDB_val key, data;
    while (mdb_cursor_get(cursor, &key, &data, MDB_NEXT) == MDB_SUCCESS) {
        struct package *pkg = package_new();
        if (!pkg) continue;

        package_metadata_free(pkg->meta);
        pkg->meta = package_metadata_from_json(data.mv_data, data.mv_size);
        if (!pkg->meta) {
            package_free(pkg);
            continue;
        }

        if (*count >= capacity) {
            capacity *= 2;
            struct package **tmp = realloc(list, capacity * sizeof(struct package *));
            if (!tmp) {
                package_free(pkg);
                break;
            }
            list = tmp;
        }

        list[(*count)++] = pkg;
    }

    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    mdb_dbi_close(env, dbi);
    return list;
}
