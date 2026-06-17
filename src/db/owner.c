// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <lmdb.h>

#include "db_priv.h"
#include "../../include/apg/db.h"

char *
db_owner(struct db_handle *db, const char *path)
{
    if (!db || !path || !db->file_owner_dbi_open)
        return NULL;

    MDB_txn *txn;
    if (mdb_txn_begin(db->env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return NULL;

    MDB_val key = {strlen(path), (void *)path};
    MDB_val val;
    char *owner = NULL;

    if (mdb_get(txn, db->file_owner_dbi, &key, &val) == MDB_SUCCESS)
        owner = strndup(val.mv_data, val.mv_size);

    mdb_txn_abort(txn);
    return owner;
}
