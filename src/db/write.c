// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <lmdb.h>

#include "db_priv.h"
#include "../../include/apg/package.h"
#include "../../include/apg/json.h"
#include "../../include/apg/journal.h"

bool
db_add(struct db_handle *db, struct package *pkg)
{
    if (!db || !pkg || !pkg->meta || db->readonly) return false;

    if (db->hooks.pre)
        db->hooks.pre(DB_OP_ADD, pkg->meta->name, db->hooks.userdata);

    // Serialize concurrent writes from multiple threads within this process.
    // Cross-process writes are serialized by LMDB's own file lock.
    pthread_mutex_lock(&db->write_lock);

    MDB_txn *txn;
    MDB_dbi dbi;
    bool ok = false;

    if (mdb_txn_begin(db->env, NULL, 0, &txn) == MDB_SUCCESS) {
        if (mdb_dbi_open(txn, NULL, 0, &dbi) == MDB_SUCCESS) {
            char *json = package_to_json(pkg);
            if (json) {
                MDB_val key  = { strlen(pkg->meta->name), pkg->meta->name };
                MDB_val data = { strlen(json), json };
                ok = mdb_put(txn, dbi, &key, &data, 0) == MDB_SUCCESS;
                free(json);
            }
            if (ok) mdb_txn_commit(txn);
            else    mdb_txn_abort(txn);
            mdb_dbi_close(db->env, dbi);
        } else {
            mdb_txn_abort(txn);
        }
    }

    pthread_mutex_unlock(&db->write_lock);

    journal_write(db->env, JOURNAL_INSTALL, pkg->meta->name, pkg->meta->version,
                  ok ? JOURNAL_STATUS_OK : JOURNAL_STATUS_FAILED);

    if (db->hooks.post)
        db->hooks.post(DB_OP_ADD, pkg->meta->name, db->hooks.userdata);

    return ok;
}

bool
db_remove(struct db_handle *db, const char *pkg_name)
{
    if (!db || !pkg_name || db->readonly) return false;

    if (db->hooks.pre)
        db->hooks.pre(DB_OP_REMOVE, pkg_name, db->hooks.userdata);

    pthread_mutex_lock(&db->write_lock);

    MDB_txn *txn;
    MDB_dbi dbi;
    bool ok = false;

    if (mdb_txn_begin(db->env, NULL, 0, &txn) == MDB_SUCCESS) {
        if (mdb_dbi_open(txn, NULL, 0, &dbi) == MDB_SUCCESS) {
            MDB_val key = { strlen(pkg_name), (void *)pkg_name };
            ok = mdb_del(txn, dbi, &key, NULL) == MDB_SUCCESS;
            if (ok) mdb_txn_commit(txn);
            else    mdb_txn_abort(txn);
            mdb_dbi_close(db->env, dbi);
        } else {
            mdb_txn_abort(txn);
        }
    }

    pthread_mutex_unlock(&db->write_lock);

    journal_write(db->env, JOURNAL_REMOVE, pkg_name, NULL,
                  ok ? JOURNAL_STATUS_OK : JOURNAL_STATUS_FAILED);

    if (db->hooks.post)
        db->hooks.post(DB_OP_REMOVE, pkg_name, db->hooks.userdata);

    return ok;
}
