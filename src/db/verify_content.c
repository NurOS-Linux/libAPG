// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "db_priv.h"
#include "../archive_priv.h"
#include "../error_priv.h"
#include "../../include/apg/json.h"
#include "../../include/apg/package.h"
#include "../../include/apg/util.h"

#define METADATA_LIMIT ((la_int64_t)1048576)

struct content_ctx
{
    struct db_content_issue *items;
    int count;
    int cap;
    bool failed;
};

enum compare_result
{
    COMPARE_SAME,
    COMPARE_DIFFERENT,
    COMPARE_ERROR,
};

static void
push_issue(struct content_ctx *ctx, db_content_kind_t kind, const char *rel)
{
    if (ctx->failed)
        return;
    if (ctx->count == ctx->cap)
    {
        int new_cap = ctx->cap ? ctx->cap * 2 : 8;
        struct db_content_issue *tmp =
            realloc(ctx->items, (size_t)new_cap * sizeof(*tmp));
        if (!tmp)
        {
            ctx->failed = true;
            return;
        }
        ctx->items = tmp;
        ctx->cap = new_cap;
    }

    char *path = concat("/", rel);
    if (!path)
    {
        ctx->failed = true;
        return;
    }
    ctx->items[ctx->count].kind = kind;
    ctx->items[ctx->count].path = path;
    ctx->count++;
}

static bool
is_conf(const struct str_list *conf, const char *rel)
{
    for (int i = 0; i < conf->count; i++)
    {
        const char *entry = conf->items[i];
        while (*entry == '/')
            entry++;
        if (strcmp(entry, rel) == 0)
            return true;
    }
    return false;
}

static enum compare_result
compare_content(struct archive *a, const char *path)
{
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd < 0)
        return COMPARE_DIFFERENT;

    enum compare_result result = COMPARE_SAME;
    unsigned char from_archive[8192];
    unsigned char from_file[8192];
    for (;;)
    {
        la_ssize_t n = archive_read_data(a, from_archive, sizeof(from_archive));
        if (n < 0)
        {
            result = COMPARE_ERROR;
            break;
        }
        if (n == 0)
        {
            if (read(fd, from_file, 1) != 0)
                result = COMPARE_DIFFERENT;
            break;
        }

        size_t got = 0;
        while (got < (size_t)n)
        {
            ssize_t r = read(fd, from_file + got, (size_t)n - got);
            if (r <= 0)
                break;
            got += (size_t)r;
        }
        if (got != (size_t)n || memcmp(from_archive, from_file, got) != 0)
        {
            result = COMPARE_DIFFERENT;
            break;
        }
    }
    close(fd);
    return result;
}

static bool
check_metadata_name(struct archive *a, struct archive_entry *entry,
                    const char *pkg_name, const char *archive_path)
{
    la_int64_t size = archive_entry_size(entry);
    if (size < 0 || size > METADATA_LIMIT)
    {
        apg_set_error("metadata.json in '%s' has an unusable size",
                      archive_path);
        return false;
    }

    char *buf = malloc((size_t)size + 1);
    if (!buf)
        return false;
    la_ssize_t n = archive_read_data(a, buf, (size_t)size);
    struct package_metadata *meta =
        n == size ? package_metadata_from_json(buf, (size_t)size) : NULL;
    free(buf);

    if (!meta || !meta->name)
    {
        apg_set_error("metadata.json in '%s' cannot be read", archive_path);
        package_metadata_free(meta);
        return false;
    }
    bool same = strcmp(meta->name, pkg_name) == 0;
    if (!same)
        apg_set_error("'%s' is an archive of '%s', not of '%s'", archive_path,
                      meta->name, pkg_name);
    package_metadata_free(meta);
    return same;
}

static void
compare_entry(struct content_ctx *ctx, struct archive *a,
              struct archive_entry *entry, const char *rel, const char *root)
{
    mode_t type = archive_entry_filetype(entry);
    char *path = concat_dirs(root, rel);
    if (!path)
    {
        ctx->failed = true;
        return;
    }

    struct stat st;
    if (lstat(path, &st) != 0)
    {
        push_issue(ctx, DB_CONTENT_MISSING, rel);
        free(path);
        return;
    }

    if (type == AE_IFLNK)
    {
        char target[PATH_MAX];
        const char *expected = archive_entry_symlink(entry);
        ssize_t len = S_ISLNK(st.st_mode)
                          ? readlink(path, target, sizeof(target) - 1)
                          : -1;
        if (len >= 0)
            target[len] = '\0';
        if (!S_ISLNK(st.st_mode))
            push_issue(ctx, DB_CONTENT_TYPE_CHANGED, rel);
        else if (len < 0 || !expected || strcmp(target, expected) != 0)
            push_issue(ctx, DB_CONTENT_MODIFIED, rel);
        free(path);
        return;
    }

    if (!S_ISREG(st.st_mode))
    {
        push_issue(ctx, DB_CONTENT_TYPE_CHANGED, rel);
        free(path);
        return;
    }

    enum compare_result result = COMPARE_DIFFERENT;
    if ((la_int64_t)st.st_size == archive_entry_size(entry))
        result = compare_content(a, path);
    if (result == COMPARE_ERROR)
        ctx->failed = true;
    else if (result == COMPARE_DIFFERENT)
        push_issue(ctx, DB_CONTENT_MODIFIED, rel);
    else if ((st.st_mode & 07777U) != (archive_entry_perm(entry) & 07777U))
        push_issue(ctx, DB_CONTENT_MODE_CHANGED, rel);
    free(path);
}

bool
db_verify_against_archive(struct db_handle *db, const char *pkg_name,
                          const char *archive_path, const char *root_path,
                          struct db_content_issue **issues, int *count)
{
    if (!db || !pkg_name || !archive_path || !root_path || !issues || !count)
        return false;
    *issues = NULL;
    *count = 0;

    struct package *installed = db_get(db, pkg_name);
    if (!installed)
    {
        apg_set_error("'%s' is not recorded in the package database", pkg_name);
        return false;
    }

    struct archive *a = apg_archive_open_read(archive_path);
    if (!a)
    {
        package_free(installed);
        return false;
    }

    struct content_ctx ctx = {0};
    bool seen_metadata = false;
    bool ok = true;
    struct archive_entry *entry;
    int r;
    while (ok && !ctx.failed &&
           (r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
        const char *name = archive_entry_pathname(entry);
        if (!name)
            continue;

        if (apg_archive_is_metadata(name))
        {
            ok = check_metadata_name(a, entry, pkg_name, archive_path);
            seen_metadata = ok;
            continue;
        }

        const char *rel = apg_archive_data_relative(name);
        mode_t type = archive_entry_filetype(entry);
        if (!rel || !*rel || archive_entry_hardlink(entry) ||
            (type != AE_IFREG && type != AE_IFLNK))
            continue;
        if (!apg_archive_is_safe_relative(rel))
        {
            apg_set_error("'%s' contains an unsafe path '%s'", archive_path,
                          name);
            ok = false;
            break;
        }
        if (is_conf(&installed->meta->conf, rel))
            continue;

        compare_entry(&ctx, a, entry, rel, root_path);
    }

    if (ok && !ctx.failed)
    {
        if (r != ARCHIVE_EOF)
        {
            apg_set_error("failed to read archive '%s': %s", archive_path,
                          archive_error_string(a));
            ok = false;
        }
        else if (!seen_metadata)
        {
            apg_set_error("'%s' has no metadata.json", archive_path);
            ok = false;
        }
    }
    if (ctx.failed && ok)
    {
        apg_set_error("cannot compare '%s' with '%s'", pkg_name, archive_path);
        ok = false;
    }

    archive_read_free(a);
    package_free(installed);

    if (!ok)
    {
        db_content_free(ctx.items, ctx.count);
        return false;
    }
    *issues = ctx.items;
    *count = ctx.count;
    return true;
}

void
db_content_free(struct db_content_issue *issues, int count)
{
    if (!issues)
        return;
    for (int i = 0; i < count; i++)
        free(issues[i].path);
    free(issues);
}

const struct db_content_issue *
db_content_issue_at(const struct db_content_issue *issues, int count, int index)
{
    if (!issues || index < 0 || index >= count)
        return NULL;
    return &issues[index];
}

db_content_kind_t
db_content_issue_kind(const struct db_content_issue *issue)
{
    return issue->kind;
}

const char *
db_content_issue_path(const struct db_content_issue *issue)
{
    return issue->path;
}
