// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <lmdb.h>
#include <stdio.h>
#include <unistd.h>

#include "db_priv.h"
#include "../../include/apg/package.h"
#include "../../include/apg/json.h"
#include "../../include/apg/journal.h"

static void
delete_file_owner_entries(struct db_handle *db, MDB_txn *txn, const char *fdata,
                          size_t flen)
{
    if (!db->file_owner_dbi_open || !fdata || flen == 0)
        return;
    const char *p = fdata, *end = fdata + flen;
    while (p < end)
    {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        if (!nl)
            break;
        if (nl > p)
        {
            MDB_val okey = {(size_t)(nl - p), (void *)p};
            mdb_del(txn, db->file_owner_dbi, &okey, NULL);
        }
        p = nl + 1;
    }
}

static char *
serialize_files(const struct str_list *files)
{
    size_t total = 1;
    for (int i = 0; i < files->count; i++)
        if (files->items[i])
            total += strlen(files->items[i]) + 1;
    char *buf = malloc(total);
    if (!buf)
        return NULL;
    char *p = buf;
    for (int i = 0; i < files->count; i++)
    {
        if (files->items[i])
        {
            size_t len = strlen(files->items[i]);
            memcpy(p, files->items[i], len);
            p += len;
            *p++ = '\n';
        }
    }
    *p = '\0';
    return buf;
}

bool
db_add(struct db_handle *db, struct package *pkg)
{
    if (!db || !pkg || !pkg->meta || db->readonly)
        return false;

    if (db->hooks.pre)
        db->hooks.pre(DB_OP_ADD, pkg->meta->name, db->hooks.userdata);

    // Serialize concurrent writes from multiple threads within this process.
    // Cross-process writes are serialized by LMDB's own file lock.
    pthread_mutex_lock(&db->write_lock);

    MDB_txn *txn;
    MDB_dbi dbi;
    bool ok = false;

    if (mdb_txn_begin(db->env, NULL, 0, &txn) == MDB_SUCCESS)
    {
        if (mdb_dbi_open(txn, NULL, 0, &dbi) == MDB_SUCCESS)
        {
            char *json = package_to_json(pkg);
            if (json)
            {
                MDB_val key = {strlen(pkg->meta->name), pkg->meta->name};
                MDB_val data = {strlen(json), json};
                ok = mdb_put(txn, dbi, &key, &data, 0) == MDB_SUCCESS;
                free(json);
            }
            if (ok && pkg->package_files.count > 0)
            {
                if (db->files_dbi_open)
                {
                    char *fdata = serialize_files(&pkg->package_files);
                    if (fdata)
                    {
                        MDB_val fkey = {strlen(pkg->meta->name),
                                        pkg->meta->name};
                        MDB_val fval = {strlen(fdata), fdata};
                        mdb_put(txn, db->files_dbi, &fkey, &fval, 0);
                        free(fdata);
                    }
                }
                if (db->file_owner_dbi_open)
                {
                    MDB_val oval = {strlen(pkg->meta->name), pkg->meta->name};
                    for (int fi = 0; fi < pkg->package_files.count; fi++)
                    {
                        const char *fp = pkg->package_files.items[fi];
                        if (!fp)
                            continue;
                        MDB_val okey = {strlen(fp), (void *)fp};
                        mdb_put(txn, db->file_owner_dbi, &okey, &oval, 0);
                    }
                }
            }
            if (ok)
                mdb_txn_commit(txn);
            else
                mdb_txn_abort(txn);
            mdb_dbi_close(db->env, dbi);
        }
        else
        {
            mdb_txn_abort(txn);
        }
    }

    pthread_mutex_unlock(&db->write_lock);

    if (!db->suppress_journal)
        journal_write(
            db->env, JOURNAL_INSTALL, pkg->meta->name, pkg->meta->version,
            ok ? JOURNAL_STATUS_OK : JOURNAL_STATUS_FAILED, getuid(), true);

    if (db->hooks.post)
        db->hooks.post(DB_OP_ADD, pkg->meta->name, db->hooks.userdata);

    return ok;
}

bool
db_remove(struct db_handle *db, const char *pkg_name)
{
    if (!db || !pkg_name || db->readonly)
        return false;

    char *version_for_journal = NULL;
    if (!db->suppress_journal)
    {
        struct package *existing = db_get(db, pkg_name);
        if (existing && existing->meta && existing->meta->version)
            version_for_journal = strdup(existing->meta->version);
        package_free(existing);
    }

    if (db->hooks.pre)
        db->hooks.pre(DB_OP_REMOVE, pkg_name, db->hooks.userdata);

    pthread_mutex_lock(&db->write_lock);

    MDB_txn *txn;
    MDB_dbi dbi;
    bool ok = false;

    if (mdb_txn_begin(db->env, NULL, 0, &txn) == MDB_SUCCESS)
    {
        if (mdb_dbi_open(txn, NULL, 0, &dbi) == MDB_SUCCESS)
        {
            MDB_val key = {strlen(pkg_name), (void *)pkg_name};
            ok = mdb_del(txn, dbi, &key, NULL) == MDB_SUCCESS;
            if (ok && db->files_dbi_open)
            {
                MDB_val fdata;
                if (mdb_get(txn, db->files_dbi, &key, &fdata) == MDB_SUCCESS)
                    delete_file_owner_entries(db, txn, fdata.mv_data,
                                              fdata.mv_size);
                mdb_del(txn, db->files_dbi, &key, NULL);
            }
            if (ok)
                mdb_txn_commit(txn);
            else
                mdb_txn_abort(txn);
            mdb_dbi_close(db->env, dbi);
        }
        else
        {
            mdb_txn_abort(txn);
        }
    }

    pthread_mutex_unlock(&db->write_lock);

    if (!db->suppress_journal)
        journal_write(db->env, JOURNAL_REMOVE, pkg_name, version_for_journal,
                      ok ? JOURNAL_STATUS_OK : JOURNAL_STATUS_FAILED, getuid(),
                      true);
    free(version_for_journal);

    if (db->hooks.post)
        db->hooks.post(DB_OP_REMOVE, pkg_name, db->hooks.userdata);

    return ok;
}
