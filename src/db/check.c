// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <lmdb.h>
#include <stdlib.h>
#include <string.h>

#include "db_priv.h"
#include "../hashmap_priv.h"
#include "../../include/apg/json.h"
#include "../../include/apg/package.h"

struct check_ctx
{
    struct db_check_issue *items;
    int count;
    int cap;
    bool failed;
};

static char *
dup_n(const char *data, size_t len)
{
    if (!data)
        return NULL;
    char *copy = malloc(len + 1);
    if (!copy)
        return NULL;
    memcpy(copy, data, len);
    copy[len] = '\0';
    return copy;
}

static void
push_issue(struct check_ctx *ctx, db_check_kind_t kind, MDB_val pkg,
           MDB_val path, MDB_val other)
{
    if (ctx->failed)
        return;
    if (ctx->count == ctx->cap)
    {
        int new_cap = ctx->cap ? ctx->cap * 2 : 8;
        struct db_check_issue *tmp =
            realloc(ctx->items, (size_t)new_cap * sizeof(*tmp));
        if (!tmp)
        {
            ctx->failed = true;
            return;
        }
        ctx->items = tmp;
        ctx->cap = new_cap;
    }

    struct db_check_issue *issue = &ctx->items[ctx->count++];
    issue->kind = kind;
    issue->pkg_name = dup_n(pkg.mv_data, pkg.mv_size);
    issue->path = dup_n(path.mv_data, path.mv_size);
    issue->other = dup_n(other.mv_data, other.mv_size);
    if ((pkg.mv_data && !issue->pkg_name) || (path.mv_data && !issue->path) ||
        (other.mv_data && !issue->other))
        ctx->failed = true;
}

static const MDB_val none = {0, NULL};

static char *
pair_key(MDB_val pkg, MDB_val path)
{
    char *key = malloc(pkg.mv_size + path.mv_size + 2);
    if (!key)
        return NULL;
    memcpy(key, pkg.mv_data, pkg.mv_size);
    key[pkg.mv_size] = '\n';
    memcpy(key + pkg.mv_size + 1, path.mv_data, path.mv_size);
    key[pkg.mv_size + 1 + path.mv_size] = '\0';
    return key;
}

static bool
has_name(const struct str_map *names, MDB_val name)
{
    char *key = dup_n(name.mv_data, name.mv_size);
    if (!key)
        return false;
    size_t unused;
    bool found = str_map_get(names, key, &unused);
    free(key);
    return found;
}

static void
check_records(struct check_ctx *ctx, MDB_txn *txn, MDB_dbi dbi,
              struct str_map *names)
{
    MDB_cursor *cursor;
    if (mdb_cursor_open(txn, dbi, &cursor) != MDB_SUCCESS)
    {
        ctx->failed = true;
        return;
    }

    MDB_val key, data;
    while (!ctx->failed &&
           mdb_cursor_get(cursor, &key, &data, MDB_NEXT) == MDB_SUCCESS)
    {
        if (data.mv_size == 0 || *(const char *)data.mv_data != '{')
            continue;

        struct package *pkg = package_from_json(data.mv_data, data.mv_size);
        if (!pkg)
        {
            push_issue(ctx, DB_CHECK_CORRUPT_RECORD, key, none, none);
            continue;
        }

        char *name = dup_n(key.mv_data, key.mv_size);
        if (!name || !str_map_set(names, name, 1))
            ctx->failed = true;

        const char *inner = pkg->meta ? pkg->meta->name : NULL;
        if (!ctx->failed && (!inner || strlen(inner) != key.mv_size ||
                             memcmp(inner, key.mv_data, key.mv_size) != 0))
        {
            MDB_val other = {inner ? strlen(inner) : 0, (void *)inner};
            push_issue(ctx, DB_CHECK_NAME_MISMATCH, key, none, other);
        }
        free(name);
        package_free(pkg);
    }
    mdb_cursor_close(cursor);
}

static void
check_file_list(struct check_ctx *ctx, MDB_txn *txn, struct db_handle *db,
                MDB_val pkg, MDB_val list, struct str_map *listed)
{
    const char *p = list.mv_data;
    const char *end = p + list.mv_size;
    while (!ctx->failed && p < end)
    {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        if (!nl)
            break;
        MDB_val path = {(size_t)(nl - p), (void *)p};
        p = nl + 1;
        if (path.mv_size == 0)
            continue;

        char *pair = pair_key(pkg, path);
        if (!pair || !str_map_set(listed, pair, 1))
        {
            free(pair);
            ctx->failed = true;
            return;
        }
        free(pair);

        MDB_val owner;
        if (mdb_get(txn, db->file_owner_dbi, &path, &owner) != MDB_SUCCESS)
            push_issue(ctx, DB_CHECK_FILE_NOT_OWNED, pkg, path, none);
        else if (owner.mv_size != pkg.mv_size ||
                 memcmp(owner.mv_data, pkg.mv_data, pkg.mv_size) != 0)
            push_issue(ctx, DB_CHECK_FILE_OWNED_BY_OTHER, pkg, path, owner);
    }
}

static void
check_file_lists(struct check_ctx *ctx, MDB_txn *txn, struct db_handle *db,
                 const struct str_map *names, struct str_map *listed)
{
    MDB_cursor *cursor;
    if (mdb_cursor_open(txn, db->files_dbi, &cursor) != MDB_SUCCESS)
    {
        ctx->failed = true;
        return;
    }

    MDB_val key, data;
    while (!ctx->failed &&
           mdb_cursor_get(cursor, &key, &data, MDB_NEXT) == MDB_SUCCESS)
    {
        if (!has_name(names, key))
            push_issue(ctx, DB_CHECK_FILES_WITHOUT_PACKAGE, key, none, none);
        else
            check_file_list(ctx, txn, db, key, data, listed);
    }
    mdb_cursor_close(cursor);
}

static void
check_owners(struct check_ctx *ctx, MDB_txn *txn, struct db_handle *db,
             const struct str_map *names, const struct str_map *listed)
{
    MDB_cursor *cursor;
    if (mdb_cursor_open(txn, db->file_owner_dbi, &cursor) != MDB_SUCCESS)
    {
        ctx->failed = true;
        return;
    }

    MDB_val path, owner;
    while (!ctx->failed &&
           mdb_cursor_get(cursor, &path, &owner, MDB_NEXT) == MDB_SUCCESS)
    {
        if (!has_name(names, owner))
        {
            push_issue(ctx, DB_CHECK_OWNER_WITHOUT_PACKAGE, owner, path, none);
            continue;
        }

        char *pair = pair_key(owner, path);
        if (!pair)
        {
            ctx->failed = true;
            break;
        }
        size_t unused;
        if (!str_map_get(listed, pair, &unused))
            push_issue(ctx, DB_CHECK_OWNER_NOT_LISTED, owner, path, none);
        free(pair);
    }
    mdb_cursor_close(cursor);
}

struct db_check_issue *
db_check(struct db_handle *db, int *count)
{
    *count = 0;
    if (!db)
        return NULL;

    struct check_ctx ctx = {0};
    struct str_map names = {0};
    struct str_map listed = {0};
    MDB_txn *txn = NULL;
    MDB_dbi dbi;
    bool dbi_open = false;

    if (!str_map_init(&names) || !str_map_init(&listed) ||
        mdb_txn_begin(db->env, NULL, MDB_RDONLY, &txn) != MDB_SUCCESS)
    {
        ctx.failed = true;
        goto done;
    }
    if (mdb_dbi_open(txn, NULL, 0, &dbi) != MDB_SUCCESS)
    {
        ctx.failed = true;
        goto done;
    }
    dbi_open = true;

    check_records(&ctx, txn, dbi, &names);
    if (db->files_dbi_open && db->file_owner_dbi_open)
    {
        check_file_lists(&ctx, txn, db, &names, &listed);
        check_owners(&ctx, txn, db, &names, &listed);
    }

done:
    if (dbi_open)
        mdb_dbi_close(db->env, dbi);
    if (txn)
        mdb_txn_abort(txn);
    str_map_free(&names);
    str_map_free(&listed);

    if (ctx.failed)
    {
        db_check_free(ctx.items, ctx.count);
        return NULL;
    }
    *count = ctx.count;
    return ctx.items;
}

void
db_check_free(struct db_check_issue *issues, int count)
{
    if (!issues)
        return;
    for (int i = 0; i < count; i++)
    {
        free(issues[i].pkg_name);
        free(issues[i].path);
        free(issues[i].other);
    }
    free(issues);
}

const struct db_check_issue *
db_check_issue_at(const struct db_check_issue *issues, int count, int index)
{
    if (!issues || index < 0 || index >= count)
        return NULL;
    return &issues[index];
}

db_check_kind_t
db_check_issue_kind(const struct db_check_issue *issue)
{
    return issue->kind;
}

const char *
db_check_issue_pkg_name(const struct db_check_issue *issue)
{
    return issue->pkg_name;
}

const char *
db_check_issue_path(const struct db_check_issue *issue)
{
    return issue->path;
}

const char *
db_check_issue_other(const struct db_check_issue *issue)
{
    return issue->other;
}
