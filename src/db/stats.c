// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <lmdb.h>

#include "db_priv.h"
#include "../../include/apg/db.h"

bool
db_stats(struct db_handle *db, struct db_stats *out)
{
    if (!db || !out)
        return false;

    out->package_count = 0;
    out->file_count = 0;

    MDB_txn *txn;
    if (mdb_txn_begin(db->env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return false;

    MDB_dbi dbi;
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS)
    {
        mdb_txn_abort(txn);
        return false;
    }

    MDB_stat st;
    if (mdb_stat(txn, dbi, &st) == MDB_SUCCESS)
        out->package_count = (int)st.ms_entries;
    mdb_dbi_close(db->env, dbi);

    if (db->files_dbi_open)
    {
        MDB_cursor *cursor;
        if (mdb_cursor_open(txn, db->files_dbi, &cursor) == MDB_SUCCESS)
        {
            MDB_val key, data;
            while (mdb_cursor_get(cursor, &key, &data, MDB_NEXT) == MDB_SUCCESS)
            {
                const char *p = data.mv_data;
                const char *end = p + data.mv_size;
                while (p < end)
                {
                    if (*p++ == '\n')
                        out->file_count++;
                }
            }
            mdb_cursor_close(cursor);
        }
    }

    mdb_txn_abort(txn);
    return true;
}
