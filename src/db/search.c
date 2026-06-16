// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <lmdb.h>

#include "db_priv.h"
#include "../../include/apg/package.h"
#include "../../include/apg/json.h"

static bool
matches(const struct package_metadata *meta, const char *query)
{
    if (meta->name && strcasestr(meta->name, query))
        return true;
    if (meta->description && strcasestr(meta->description, query))
        return true;
    return false;
}

struct package **
db_search(struct db_handle *db, const char *query, int *count)
{
    *count = 0;
    if (!db || !query || !*query)
        return NULL;

    MDB_txn *txn;
    MDB_dbi dbi;

    if (mdb_txn_begin(db->env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return NULL;
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS)
    {
        mdb_txn_abort(txn);
        return NULL;
    }

    MDB_cursor *cursor;
    if (mdb_cursor_open(txn, dbi, &cursor) != MDB_SUCCESS)
    {
        mdb_txn_abort(txn);
        mdb_dbi_close(db->env, dbi);
        return NULL;
    }

    int cap = 8;
    struct package **list = malloc(cap * sizeof(*list));
    if (!list)
        goto cleanup;

    MDB_val key, data;
    while (mdb_cursor_get(cursor, &key, &data, MDB_NEXT) == MDB_SUCCESS)
    {
        struct package *pkg = package_from_json(data.mv_data, data.mv_size);
        if (!pkg)
            continue;
        if (!pkg->meta || !matches(pkg->meta, query))
        {
            package_free(pkg);
            continue;
        }

        if (*count == cap)
        {
            cap *= 2;
            struct package **tmp = realloc(list, cap * sizeof(*tmp));
            if (!tmp)
            {
                package_free(pkg);
                break;
            }
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
