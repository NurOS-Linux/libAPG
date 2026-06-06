// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <lmdb.h>

#include "db_priv.h"
#include "../../include/apg/package.h"
#include "../../include/apg/json.h"

static struct str_list
deserialize_files(const char *data, size_t len)
{
    struct str_list list = {0};
    if (!data || len == 0) return list;
    int count = 0;
    for (size_t i = 0; i < len; i++)
        if (data[i] == '\n') count++;
    if (count == 0) return list;
    list.items = malloc(count * sizeof(char *));
    if (!list.items) return list;
    const char *p = data, *end = data + len;
    int idx = 0;
    while (p < end && idx < count) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        if (!nl) break;
        if (nl > p) list.items[idx++] = strndup(p, (size_t)(nl - p));
        p = nl + 1;
    }
    list.count = idx;
    return list;
}

static struct str_list
load_package_files(struct db_handle *db, const char *pkg_name)
{
    struct str_list files = {0};
    if (!db->files_dbi_open) return files;
    MDB_txn *txn;
    if (mdb_txn_begin(db->env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return files;
    MDB_val key  = { strlen(pkg_name), (void *)pkg_name };
    MDB_val data;
    if (mdb_get(txn, db->files_dbi, &key, &data) == MDB_SUCCESS)
        files = deserialize_files(data.mv_data, data.mv_size);
    mdb_txn_abort(txn);
    return files;
}

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

    struct package *pkg = package_from_json(data.mv_data, data.mv_size);
    if (pkg && pkg->meta && pkg->meta->name)
        pkg->package_files = load_package_files(db, pkg->meta->name);

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
        struct package *pkg = package_from_json(data.mv_data, data.mv_size);
        if (!pkg) continue;
        if (!pkg->meta) { package_free(pkg); continue; }
        if (pkg->meta->name)
            pkg->package_files = load_package_files(db, pkg->meta->name);

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
