// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <pthread.h>
#include <lmdb.h>

#include "db_priv.h"

static struct db_handle *
open_env(const char *path, unsigned int extra_flags)
{
    struct db_handle *db = calloc(1, sizeof(*db));
    if (!db)
        return NULL;

    if (mdb_env_create(&db->env) != MDB_SUCCESS)
        goto fail;
    mdb_env_set_maxdbs(db->env, 2);
    mdb_env_set_maxreaders(db->env, 256);
    mdb_env_set_mapsize(db->env, 10485760);

    // MDB_NOTLS: read transactions are not tied to a thread, safe for
    // concurrent use
    if (mdb_env_open(db->env, path, extra_flags | MDB_NOTLS, 0664) !=
        MDB_SUCCESS)
        goto fail;

    pthread_mutex_init(&db->write_lock, NULL);
    return db;

fail:
    if (db->env)
        mdb_env_close(db->env);
    free(db);
    return NULL;
}

struct db_handle *
db_open(const char *path)
{
    struct db_handle *db = open_env(path, 0);
    if (!db)
        return NULL;

    MDB_txn *txn;
    if (mdb_txn_begin(db->env, NULL, 0, &txn) == MDB_SUCCESS)
    {
        if (mdb_dbi_open(txn, "files", MDB_CREATE, &db->files_dbi) ==
            MDB_SUCCESS)
        {
            mdb_txn_commit(txn);
            db->files_dbi_open = true;
        }
        else
        {
            mdb_txn_abort(txn);
        }
    }
    return db;
}

struct db_handle *
db_open_readonly(const char *path)
{
    struct db_handle *db = open_env(path, MDB_RDONLY);
    if (!db)
        return NULL;
    db->readonly = true;

    MDB_txn *txn;
    if (mdb_txn_begin(db->env, NULL, MDB_RDONLY, &txn) == MDB_SUCCESS)
    {
        if (mdb_dbi_open(txn, "files", 0, &db->files_dbi) == MDB_SUCCESS)
            db->files_dbi_open = true;
        mdb_txn_abort(txn);
    }
    return db;
}

void
db_close(struct db_handle *db)
{
    if (!db)
        return;
    mdb_env_close(db->env);
    pthread_mutex_destroy(&db->write_lock);
    free(db);
}

void
db_set_hooks(struct db_handle *db, const struct db_hooks *hooks)
{
    if (!db || !hooks)
        return;
    db->hooks = *hooks;
}
