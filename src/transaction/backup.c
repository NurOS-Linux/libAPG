// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "backup_priv.h"
#include "../../include/apg/copy.h"
#include "../../include/apg/scripts.h"
#include "../../include/apg/util.h"
#include "../error_priv.h"
#include "../util_priv.h"

static _Atomic uint64_t g_backup_seq = 0;

static bool
make_parent_dirs(const char *path)
{
    char *copy = strdup(path);
    if (!copy)
        return false;
    char *slash = strrchr(copy, '/');
    bool ok = true;
    if (slash && slash != copy)
    {
        *slash = '\0';
        ok = apg_make_dirs(copy);
    }
    free(copy);
    return ok;
}

static bool
copy_entry(const char *src, const char *dst)
{
    struct stat st;
    if (lstat(src, &st) != 0)
        return true;

    if (!make_parent_dirs(dst))
        return false;
    unlink(dst);

    if (S_ISLNK(st.st_mode))
    {
        char target[PATH_MAX];
        ssize_t len = readlink(src, target, sizeof(target) - 1);
        if (len < 0)
        {
            apg_set_error("cannot read link '%s': %s", src, strerror(errno));
            return false;
        }
        target[len] = '\0';
        if (symlink(target, dst) != 0)
        {
            apg_set_error("cannot create link '%s': %s", dst, strerror(errno));
            return false;
        }
        return true;
    }
    if (S_ISREG(st.st_mode))
        return copy_file(src, dst);
    return true;
}

static char *
backup_path(const struct upgrade_backup *backup, const char *rel)
{
    char *files = concat_dirs(backup->dir, "files");
    if (!files)
        return NULL;
    char *path = concat_dirs(files, rel);
    free(files);
    return path;
}

bool
upgrade_backup_create(struct db_handle *db, const char *pkg_name,
                      const char *root_path, struct upgrade_backup *out)
{
    memset(out, 0, sizeof(*out));
    out->previous = db_get(db, pkg_name);
    if (!out->previous)
        return true;

    char leaf[64];
    (void)snprintf(leaf, sizeof(leaf), "backup-%d-%" PRIu64, (int)getpid(),
                   ++g_backup_seq);
    char *base = concat_dirs(root_path, APG_TMP_DIR);
    out->dir = base ? concat_dirs(base, leaf) : NULL;
    free(base);
    if (!out->dir || !apg_make_dirs(out->dir))
        return false;

    const struct str_list *files = &out->previous->package_files;
    for (int i = 0; i < files->count; i++)
    {
        if (!files->items[i])
            continue;
        char *src = concat_dirs(root_path, files->items[i]);
        char *dst = backup_path(out, files->items[i]);
        bool ok = src && dst && copy_entry(src, dst);
        free(src);
        free(dst);
        if (!ok)
            return false;
    }

    char *store = scripts_store_path(root_path, pkg_name);
    struct stat st;
    if (store && stat(store, &st) == 0 && S_ISDIR(st.st_mode))
    {
        char *saved = concat_dirs(out->dir, "scripts");
        out->had_scripts = saved && copy_dir(store, saved);
        free(saved);
        if (!out->had_scripts)
        {
            free(store);
            return false;
        }
    }
    free(store);
    return true;
}

void
upgrade_backup_restore(struct db_handle *db,
                       const struct upgrade_backup *backup,
                       const struct package *new_pkg, const char *root_path)
{
    const struct str_list *added = &new_pkg->package_files;
    for (int i = 0; i < added->count; i++)
    {
        if (!added->items[i])
            continue;
        char *path = concat_dirs(root_path, added->items[i]);
        if (path)
        {
            unlink(path);
            free(path);
        }
    }

    const struct str_list *files = &backup->previous->package_files;
    for (int i = 0; i < files->count; i++)
    {
        if (!files->items[i])
            continue;
        char *src = backup_path(backup, files->items[i]);
        char *dst = concat_dirs(root_path, files->items[i]);
        if (src && dst)
            (void)copy_entry(src, dst);
        free(src);
        free(dst);
    }

    char *store = scripts_store_path(root_path, backup->previous->meta->name);
    if (store)
    {
        remove_dir_recursive(store);
        if (backup->had_scripts)
        {
            char *saved = concat_dirs(backup->dir, "scripts");
            if (saved)
                (void)copy_dir(saved, store);
            free(saved);
        }
        free(store);
    }

    (void)db_add(db, backup->previous);
}

void
upgrade_backup_discard(struct upgrade_backup *backup)
{
    if (backup->dir)
        remove_dir_recursive(backup->dir);
    free(backup->dir);
    package_free(backup->previous);
    memset(backup, 0, sizeof(*backup));
}
