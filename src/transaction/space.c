// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "trans_priv.h"
#include "../archive_priv.h"
#include "../error_priv.h"
#include "../../include/apg/package.h"
#include "../../include/apg/scripts.h"
#include "../../include/apg/util.h"

static uint64_t
round_to_blocks(uint64_t size)
{
    return (size + APG_SPACE_BLOCK - 1) / APG_SPACE_BLOCK * APG_SPACE_BLOCK;
}

static uint64_t
regular_file_bytes(const char *path)
{
    struct stat st;
    if (lstat(path, &st) != 0 || !S_ISREG(st.st_mode))
        return 0;
    return round_to_blocks((uint64_t)st.st_size);
}

static uint64_t
package_files_bytes(const char *root_path, const struct str_list *files)
{
    uint64_t total = 0;
    for (int i = 0; i < files->count; i++)
    {
        char *path = concat_dirs(root_path, files->items[i]);
        if (!path)
            continue;
        total += regular_file_bytes(path);
        free(path);
    }
    return total;
}

static uint64_t
directory_bytes(const char *dir)
{
    int count = 0;
    char **files = collect_files(dir, &count);
    uint64_t total = 0;
    for (int i = 0; i < count; i++)
    {
        char *path = concat_dirs(dir, files[i]);
        if (path)
        {
            total += regular_file_bytes(path);
            free(path);
        }
        free(files[i]);
    }
    free(files);
    return total;
}

static bool
filesystem_of(const char *path, dev_t *dev, uint64_t *available)
{
    char probe[PATH_MAX];
    if (strlen(path) >= sizeof(probe))
        return false;
    memcpy(probe, path, strlen(path) + 1);

    struct stat st;
    while (stat(probe, &st) != 0)
    {
        char *slash = strrchr(probe, '/');
        if (!slash)
            return false;
        if (slash == probe)
        {
            slash[1] = '\0';
            if (stat(probe, &st) != 0)
                return false;
            break;
        }
        *slash = '\0';
    }

    struct statvfs vfs;
    if (statvfs(probe, &vfs) != 0)
        return false;

    *dev = st.st_dev;
    *available = (uint64_t)vfs.f_bavail * (uint64_t)vfs.f_frsize;
    return true;
}

bool
trans_estimate_space(const struct apg_trans *trans, const char *root_path,
                     uint64_t *needed, uint64_t *available)
{
    if (!trans || !root_path || !needed || !available)
        return false;
    if (!trans->prepared)
    {
        apg_set_error("cannot estimate disk space: the transaction is not "
                      "prepared");
        return false;
    }

    uint64_t installed_total = 0;
    uint64_t largest_extraction = 0;
    uint64_t backup_total = 0;

    for (size_t i = 0; i < trans->plan_count; i++)
    {
        const struct trans_step *step = &trans->plan[i];
        if (step->op == TRANS_OP_REMOVE)
            continue;

        const struct package *pkg = trans->plan_pkgs[i];
        if (!pkg || !pkg->pkg_path)
        {
            apg_set_error("cannot estimate disk space: '%s' has no archive "
                          "path",
                          step->pkg_name);
            return false;
        }

        uint64_t extracted = 0;
        uint64_t installed = 0;
        if (!apg_archive_measure(pkg->pkg_path, &extracted, &installed))
            return false;
        installed_total += installed;
        if (extracted > largest_extraction)
            largest_extraction = extracted;

        if (step->op != TRANS_OP_UPGRADE)
            continue;

        struct package *previous = db_get(trans->db, step->pkg_name);
        if (!previous)
            continue;
        backup_total +=
            package_files_bytes(root_path, &previous->package_files);
        char *store = scripts_store_path(root_path, step->pkg_name);
        if (store)
        {
            backup_total += directory_bytes(store);
            free(store);
        }
        package_free(previous);
    }

    uint64_t root_need = installed_total;
    uint64_t tmp_need = largest_extraction + backup_total;

    char *tmp_dir = concat_dirs(root_path, APG_TMP_DIR);
    dev_t root_dev;
    dev_t tmp_dev;
    uint64_t root_avail;
    uint64_t tmp_avail;
    bool ok = tmp_dir && filesystem_of(root_path, &root_dev, &root_avail) &&
              filesystem_of(tmp_dir, &tmp_dev, &tmp_avail);
    free(tmp_dir);
    if (!ok)
    {
        apg_set_error("cannot estimate disk space: cannot read file system "
                      "information for '%s'",
                      root_path);
        return false;
    }

    if (root_dev == tmp_dev)
    {
        *needed = root_need + tmp_need;
        *available = root_avail;
    }
    else if (tmp_avail + root_need < root_avail + tmp_need)
    {
        *needed = tmp_need;
        *available = tmp_avail;
    }
    else
    {
        *needed = root_need;
        *available = root_avail;
    }
    return true;
}
