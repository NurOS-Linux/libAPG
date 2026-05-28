// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <lmdb.h>

#include "db_priv.h"
#include "../../include/apg/package.h"
#include "../../include/apg/json.h"

// Read path acquires no write_lock — LMDB MVCC allows concurrent readers
// regardless of any in-progress write transaction.

struct package *
db_get(struct db_handle *db, const char *name)
{
    if (!db || !name) return NULL;

    MDB_txn *txn;
    MDB_dbi dbi;

    if (mdb_txn_begin(db->env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return NULL;
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        return NULL;
    }

    MDB_val key  = { strlen(name), (void *)name };
    MDB_val data;

    if (mdb_get(txn, dbi, &key, &data) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        mdb_dbi_close(db->env, dbi);
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
    mdb_dbi_close(db->env, dbi);
    return pkg;
}

struct package **
db_list(struct db_handle *db, int *count)
{
    *count = 0;
    if (!db) return NULL;

    MDB_txn *txn;
    MDB_dbi dbi;

    if (mdb_txn_begin(db->env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return NULL;
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        return NULL;
    }

    MDB_cursor *cursor;
    if (mdb_cursor_open(txn, dbi, &cursor) != MDB_SUCCESS) {
        mdb_txn_abort(txn);
        mdb_dbi_close(db->env, dbi);
        return NULL;
    }

    int cap = 16;
    struct package **list = malloc(cap * sizeof(*list));
    if (!list) goto cleanup;

    MDB_val key, data;
    while (mdb_cursor_get(cursor, &key, &data, MDB_NEXT) == MDB_SUCCESS) {
        struct package *pkg = package_new();
        if (!pkg) continue;

        package_metadata_free(pkg->meta);
        pkg->meta = package_metadata_from_json(data.mv_data, data.mv_size);
        if (!pkg->meta) { package_free(pkg); continue; }

        if (*count == cap) {
            cap *= 2;
            struct package **tmp = realloc(list, cap * sizeof(*tmp));
            if (!tmp) { package_free(pkg); break; }
            list = tmp;
        }
        list[(*count)++] = pkg;
    }

cleanup:
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    mdb_dbi_close(db->env, dbi);
    return list;
}
