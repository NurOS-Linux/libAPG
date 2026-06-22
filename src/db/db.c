// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <pthread.h>
#include <lmdb.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdio.h>

#include "db_priv.h"

static int
acquire_write_lock(const char *db_path)
{
    char lock_path[4096];
    snprintf(lock_path, sizeof(lock_path), "%s/db.lock", db_path);

    int fd = open(lock_path, O_CREAT | O_RDWR, 0644);
    if (fd < 0)
        return -1;

    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

static struct db_handle *
open_env(const char *path, unsigned int extra_flags)
{
    struct db_handle *db = calloc(1, sizeof(*db));
    if (!db)
        return NULL;

    db->lock_fd = -1;

    if (mdb_env_create(&db->env) != MDB_SUCCESS)
        goto fail;
    mdb_env_set_maxdbs(db->env, 3);
    mdb_env_set_maxreaders(db->env, 256);
    mdb_env_set_mapsize(db->env, APG_DB_MAPSIZE);

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

    db->lock_fd = acquire_write_lock(path);
    if (db->lock_fd < 0)
    {
        mdb_env_close(db->env);
        pthread_mutex_destroy(&db->write_lock);
        free(db);
        return NULL;
    }

    MDB_txn *txn;
    if (mdb_txn_begin(db->env, NULL, 0, &txn) == MDB_SUCCESS)
    {
        if (mdb_dbi_open(txn, "files", MDB_CREATE, &db->files_dbi) ==
            MDB_SUCCESS)
            db->files_dbi_open = true;
        if (mdb_dbi_open(txn, "file_owner", MDB_CREATE, &db->file_owner_dbi) ==
            MDB_SUCCESS)
            db->file_owner_dbi_open = true;
        if (db->files_dbi_open || db->file_owner_dbi_open)
            mdb_txn_commit(txn);
        else
            mdb_txn_abort(txn);
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
        if (mdb_dbi_open(txn, "file_owner", 0, &db->file_owner_dbi) ==
            MDB_SUCCESS)
            db->file_owner_dbi_open = true;
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
    if (db->lock_fd >= 0)
    {
        flock(db->lock_fd, LOCK_UN);
        close(db->lock_fd);
    }
    free(db);
}

void
db_set_hooks(struct db_handle *db, const struct db_hooks *hooks)
{
    if (!db || !hooks)
        return;
    db->hooks = *hooks;
}
