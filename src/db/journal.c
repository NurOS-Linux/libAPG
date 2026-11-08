// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <lmdb.h>

#include "db_priv.h"
#include "../../include/apg/journal.h"

struct journal_entry
{
    journal_op_t op;
    char *pkg_name;
    char *pkg_version;
    time_t timestamp;
    journal_status_t status;
    uid_t uid;
    bool explicit_op;
};

static _Atomic uint32_t seq_counter = 0;

// Key layout: 8 bytes big-endian timestamp || 4 bytes big-endian sequence.
// Big-endian ensures lexicographic order == chronological order.
static void
pack_key(uint8_t out[12], time_t ts, uint32_t seq)
{
    uint64_t t = (uint64_t)ts;
    out[7] = t & 0xffU;
    t >>= 8U;
    out[6] = t & 0xffU;
    t >>= 8U;
    out[5] = t & 0xffU;
    t >>= 8U;
    out[4] = t & 0xffU;
    t >>= 8U;
    out[3] = t & 0xffU;
    t >>= 8U;
    out[2] = t & 0xffU;
    t >>= 8U;
    out[1] = t & 0xffU;
    t >>= 8U;
    out[0] = t & 0xffU;
    out[11] = seq & 0xffU;
    seq >>= 8U;
    out[10] = seq & 0xffU;
    seq >>= 8U;
    out[9] = seq & 0xffU;
    seq >>= 8U;
    out[8] = seq & 0xffU;
}

static time_t
unpack_ts(const uint8_t *key)
{
    uint64_t t = 0;
    for (int i = 0; i < 8; i++)
        t = (t << 8U) | (uint64_t)key[i];
    return (time_t)t;
}

static const char *
op_to_str(journal_op_t op)
{
    if (op == JOURNAL_INSTALL)
        return "install";
    if (op == JOURNAL_ROLLBACK)
        return "rollback";
    return "remove";
}

static const char *
status_to_str(journal_status_t st)
{
    return st == JOURNAL_STATUS_OK ? "ok" : "failed";
}

static char *
escape_field(const char *src)
{
    size_t len = 0;
    for (const char *p = src; *p; p++)
        len += (*p == '|' || *p == '%') ? 3 : 1;

    char *out = malloc(len + 1);
    if (!out)
        return NULL;

    char *w = out;
    for (const char *p = src; *p; p++)
    {
        if (*p == '|')
        {
            memcpy(w, "%7C", 3);
            w += 3;
        }
        else if (*p == '%')
        {
            memcpy(w, "%25", 3);
            w += 3;
        }
        else
        {
            *w++ = *p;
        }
    }
    *w = '\0';
    return out;
}

static void
unescape_field(char *s)
{
    char *w = s;
    for (const char *r = s; *r; r++)
    {
        if (r[0] == '%' && r[1] == '7' && r[2] == 'C')
        {
            *w++ = '|';
            r += 2;
        }
        else if (r[0] == '%' && r[1] == '2' && r[2] == '5')
        {
            *w++ = '%';
            r += 2;
        }
        else
        {
            *w++ = *r;
        }
    }
    *w = '\0';
}

int
apg_journal_put(MDB_txn *txn, journal_op_t op, const char *pkg_name,
                const char *pkg_version, journal_status_t status, uid_t uid,
                bool explicit_op)
{
    char *name_field = escape_field(pkg_name ? pkg_name : "-");
    char *version_field = escape_field(pkg_version ? pkg_version : "-");
    if (!name_field || !version_field)
    {
        free(name_field);
        free(version_field);
        return ENOMEM;
    }

    size_t val_cap = strlen(name_field) + strlen(version_field) + 64;
    char *val = malloc(val_cap);
    if (!val)
    {
        free(name_field);
        free(version_field);
        return ENOMEM;
    }
    (void)snprintf(val, val_cap, "%s|%s|%s|%s|%u|%d", op_to_str(op), name_field,
                   version_field, status_to_str(status), (unsigned)uid,
                   explicit_op ? 1 : 0);
    free(name_field);
    free(version_field);

    MDB_dbi dbi;
    int rc = mdb_dbi_open(txn, "journal", MDB_CREATE, &dbi);
    if (rc != MDB_SUCCESS)
    {
        free(val);
        return rc;
    }

    uint8_t raw_key[12];
    pack_key(raw_key, time(NULL), atomic_fetch_add(&seq_counter, 1));

    MDB_val k = {sizeof(raw_key), raw_key};
    MDB_val v = {strlen(val), val};

    rc = mdb_put(txn, dbi, &k, &v, 0);
    free(val);
    return rc;
}

bool
journal_write(MDB_env *env, journal_op_t op, const char *pkg_name,
              const char *pkg_version, journal_status_t status, uid_t uid,
              bool explicit_op)
{
    MDB_txn *txn;
    if (mdb_txn_begin(env, NULL, 0, &txn) != MDB_SUCCESS)
        return false;

    if (apg_journal_put(txn, op, pkg_name, pkg_version, status, uid,
                        explicit_op) != MDB_SUCCESS)
    {
        mdb_txn_abort(txn);
        return false;
    }

    mdb_txn_commit(txn);
    return true;
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

        char *fields[6] = {raw, NULL, NULL, NULL, NULL, NULL};
        int fi = 0;
        for (char *p = raw; *p && fi < 5; p++)
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

        if (strcmp(fields[0], "install") == 0)
            e->op = JOURNAL_INSTALL;
        else if (strcmp(fields[0], "rollback") == 0)
            e->op = JOURNAL_ROLLBACK;
        else
            e->op = JOURNAL_REMOVE;
        unescape_field(fields[1]);
        unescape_field(fields[2]);
        e->pkg_name = strcmp(fields[1], "-") == 0 ? NULL : strdup(fields[1]);
        e->pkg_version = strcmp(fields[2], "-") == 0 ? NULL : strdup(fields[2]);
        e->status = strcmp(fields[3], "ok") == 0 ? JOURNAL_STATUS_OK
                                                 : JOURNAL_STATUS_FAILED;
        e->uid =
            (fi >= 4 && fields[4]) ? (uid_t)strtoul(fields[4], NULL, 10) : 0;
        e->explicit_op = (fi >= 5 && fields[5]) ? fields[5][0] == '1' : false;
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

journal_op_t
journal_entry_op(const struct journal_entry *e)
{
    return e->op;
}

const char *
journal_entry_pkg_name(const struct journal_entry *e)
{
    return e->pkg_name;
}

const char *
journal_entry_pkg_version(const struct journal_entry *e)
{
    return e->pkg_version;
}

time_t
journal_entry_timestamp(const struct journal_entry *e)
{
    return e->timestamp;
}

journal_status_t
journal_entry_status(const struct journal_entry *e)
{
    return e->status;
}

uid_t
journal_entry_uid(const struct journal_entry *e)
{
    return e->uid;
}

bool
journal_entry_explicit(const struct journal_entry *e)
{
    return e->explicit_op;
}
