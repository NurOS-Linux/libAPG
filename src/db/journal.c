// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lmdb.h>

#include "../../include/apg/journal.h"

static uint32_t seq_counter = 0;

// Key layout: 8 bytes big-endian timestamp || 4 bytes big-endian sequence.
// Big-endian ensures lexicographic order == chronological order.
static void
pack_key(uint8_t out[12], time_t ts, uint32_t seq)
{
    uint64_t t = (uint64_t)ts;
    out[7] = t & 0xff;
    t >>= 8;
    out[6] = t & 0xff;
    t >>= 8;
    out[5] = t & 0xff;
    t >>= 8;
    out[4] = t & 0xff;
    t >>= 8;
    out[3] = t & 0xff;
    t >>= 8;
    out[2] = t & 0xff;
    t >>= 8;
    out[1] = t & 0xff;
    t >>= 8;
    out[0] = t & 0xff;
    out[11] = seq & 0xff;
    seq >>= 8;
    out[10] = seq & 0xff;
    seq >>= 8;
    out[9] = seq & 0xff;
    seq >>= 8;
    out[8] = seq & 0xff;
}

static time_t
unpack_ts(const uint8_t *key)
{
    uint64_t t = 0;
    for (int i = 0; i < 8; i++)
        t = (t << 8) | key[i];
    return (time_t)t;
}

static const char *
op_to_str(journal_op_t op)
{
    return op == JOURNAL_INSTALL ? "install" : "remove";
}

static const char *
status_to_str(journal_status_t st)
{
    return st == JOURNAL_STATUS_OK ? "ok" : "failed";
}

bool
journal_write(MDB_env *env, journal_op_t op, const char *pkg_name,
              const char *pkg_version, journal_status_t status)
{
    MDB_txn *txn;
    MDB_dbi dbi;

    if (mdb_txn_begin(env, NULL, 0, &txn) != MDB_SUCCESS)
        return false;
    if (mdb_dbi_open(txn, "journal", MDB_CREATE, &dbi) != MDB_SUCCESS)
    {
        mdb_txn_abort(txn);
        return false;
    }

    uint8_t raw_key[12];
    pack_key(raw_key, time(NULL), seq_counter++);

    char val[512];
    snprintf(val, sizeof(val), "%s|%s|%s|%s", op_to_str(op),
             pkg_name ? pkg_name : "-", pkg_version ? pkg_version : "-",
             status_to_str(status));

    MDB_val k = {sizeof(raw_key), raw_key};
    MDB_val v = {strlen(val), val};

    bool ok = mdb_put(txn, dbi, &k, &v, 0) == MDB_SUCCESS;
    if (ok)
        mdb_txn_commit(txn);
    else
        mdb_txn_abort(txn);

    mdb_dbi_close(env, dbi);
    return ok;
}

struct journal_entry **
journal_read_all(MDB_env *env, int *count)
{
    *count = 0;
    MDB_txn *txn;
    MDB_dbi dbi;

    if (mdb_txn_begin(env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
        return NULL;

    int rc = mdb_dbi_open(txn, "journal", 0, &dbi);
    if (rc != MDB_SUCCESS)
    {
        mdb_txn_abort(txn);
        return NULL;
    }

    MDB_cursor *cursor;
    if (mdb_cursor_open(txn, dbi, &cursor) != MDB_SUCCESS)
    {
        mdb_txn_abort(txn);
        mdb_dbi_close(env, dbi);
        return NULL;
    }

    int cap = 16;
    struct journal_entry **list = malloc(cap * sizeof(*list));
    if (!list)
        goto cleanup;

    MDB_val k, v;
    while (mdb_cursor_get(cursor, &k, &v, MDB_NEXT) == MDB_SUCCESS)
    {
        if (k.mv_size < 12)
            continue;

        char *raw = malloc(v.mv_size + 1);
        if (!raw)
            continue;
        memcpy(raw, v.mv_data, v.mv_size);
        raw[v.mv_size] = '\0';

        char *fields[4] = {raw, NULL, NULL, NULL};
        int fi = 0;
        for (char *p = raw; *p && fi < 3; p++)
        {
            if (*p == '|')
            {
                *p = '\0';
                fields[++fi] = p + 1;
            }
        }
        if (fi < 3)
        {
            free(raw);
            continue;
        }

        struct journal_entry *e = malloc(sizeof(*e));
        if (!e)
        {
            free(raw);
            continue;
        }

        e->op = strcmp(fields[0], "install") == 0 ? JOURNAL_INSTALL
                                                  : JOURNAL_REMOVE;
        e->pkg_name = strcmp(fields[1], "-") == 0 ? NULL : strdup(fields[1]);
        e->pkg_version = strcmp(fields[2], "-") == 0 ? NULL : strdup(fields[2]);
        e->status = strcmp(fields[3], "ok") == 0 ? JOURNAL_STATUS_OK
                                                 : JOURNAL_STATUS_FAILED;
        e->timestamp = unpack_ts((const uint8_t *)k.mv_data);
        free(raw);

        if (*count == cap)
        {
            cap *= 2;
            struct journal_entry **tmp = realloc(list, cap * sizeof(*tmp));
            if (!tmp)
            {
                journal_entry_free(e);
                break;
            }
            list = tmp;
        }
        list[(*count)++] = e;
    }

cleanup:
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    mdb_dbi_close(env, dbi);
    return list;
}

void
journal_entry_free(struct journal_entry *e)
{
    if (!e)
        return;
    free(e->pkg_name);
    free(e->pkg_version);
    free(e);
}

void
journal_free_all(struct journal_entry **entries, int count)
{
    if (!entries)
        return;
    for (int i = 0; i < count; i++)
        journal_entry_free(entries[i]);
    free(entries);
}
