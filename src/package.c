// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <inttypes.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/apg/package.h"
#include "../include/apg/version.h"
#include "../include/apg/install.h"
#include "../include/apg/util.h"
#include "../include/apg/scripts.h"
#include "../include/apg/archive.h"
#include "../include/apg/json.h"
#include "error_priv.h"
#include "util_priv.h"
#include "script_priv.h"

static const char *tmp_path = APG_TMP_DIR "/";
static _Atomic uint64_t g_extract_seq = 0;

static char *
unique_tmp_dir(const char *root_path)
{
    char *base = concat_dirs(root_path, tmp_path);
    if (!base)
        return NULL;

    char leaf[64];
    (void)snprintf(leaf, sizeof(leaf), "pkg-%d-%" PRIu64, (int)getpid(),
                   ++g_extract_seq);

    char *unique = concat_dirs(base, leaf);
    free(base);
    if (!unique)
        return NULL;

    if (!apg_make_dirs(unique))
    {
        free(unique);
        return NULL;
    }
    return unique;
}

struct package_metadata *
package_metadata_new(void)
{
    struct package_metadata *meta = calloc(1, sizeof(*meta));
    if (!meta)
        return NULL;
    return meta;
}

struct package *
package_new(void)
{
    // ReSharper disable once CppDFAMemoryLeak
    struct package *pkg = calloc(1, sizeof(*pkg));
    if (!pkg)
        return NULL;

    pkg->meta = package_metadata_new();
    if (!pkg->meta)
    {
        free(pkg);
        return NULL;
    }
    return pkg;
}

void
str_list_free(struct str_list *list)
{
    if (!list)
        return;
    for (int i = 0; i < list->count; i++)
        free(list->items[i]);
    free(list->items);
}

void
package_metadata_free(struct package_metadata *meta)
{
    if (!meta)
        return;

    free(meta->name);
    free(meta->version);
    free(meta->type);
    free(meta->architecture);
    free(meta->description);
    free(meta->maintainer);
    free(meta->license);
    free(meta->homepage);

    str_list_free(&meta->tags);
    dep_constraint_list_free(&meta->dependencies);
    str_list_free(&meta->conflicts);
    str_list_free(&meta->provides);
    str_list_free(&meta->replaces);
    str_list_free(&meta->conf);

    free(meta);
}

void
package_free(struct package *pkg)
{
    if (!pkg)
        return;
    package_metadata_free(pkg->meta);
    free((void *)pkg->pkg_path);
    str_list_free(&pkg->package_files);
    free(pkg);
}

bool
install_package(struct package *pkg)
{
    return install_package_in_root(pkg, "/");
}

bool
install_package_in_root(struct package *pkg, const char *root_path)
{
    return apg_install_package_in_root(pkg, root_path, false);
}

bool
apg_install_package_in_root(struct package *pkg, const char *root_path,
                            bool strict)
{
    apg_clear_error();

    char *real_tmp = unique_tmp_dir(root_path);
    if (!real_tmp)
        return false;

    if (!unarchive_package_in_root(pkg, real_tmp))
    {
        remove_dir_recursive(real_tmp);
        free(real_tmp);
        return false;
    }

    if (!apg_run_script(real_tmp, "pre-install", root_path, strict))
    {
        remove_dir_recursive(real_tmp);
        free(real_tmp);
        return false;
    }

    if (!install_data_dir(real_tmp, root_path))
    {
        remove_dir_recursive(real_tmp);
        free(real_tmp);
        return false;
    }

    char *data_src = concat_dirs(real_tmp, "data");
    if (data_src)
    {
        int file_count = 0;
        char **files = collect_files(data_src, &file_count);
        free(data_src);
        if (files)
        {
            str_list_free(&pkg->package_files);
            pkg->package_files.items = files;
            pkg->package_files.count = file_count;
        }
    }

    install_home_dir(real_tmp);

    if (!apg_run_script(real_tmp, "post-install", root_path, strict))
    {
        rollback_install(real_tmp, root_path);
        remove_dir_recursive(real_tmp);
        free(real_tmp);
        return false;
    }

    scripts_persist(real_tmp, root_path, pkg->meta->name);

    remove_dir_recursive(real_tmp);
    free(real_tmp);
    return true;
}

bool
package_collect_files(struct package *pkg, const char *root_path)
{
    (void)root_path;
    apg_clear_error();

    if (!pkg || !pkg->pkg_path)
        return false;

    char **files = NULL;
    int file_count = 0;
    if (!package_list_files(pkg->pkg_path, &files, &file_count))
        return false;

    str_list_free(&pkg->package_files);
    pkg->package_files.items = files;
    pkg->package_files.count = file_count;
    return true;
}

struct package *
parse_package(const char *path, const char *root_path)
{
    (void)root_path;
    apg_clear_error();

    struct package *pkg = package_new();
    if (!pkg)
        return NULL;

    pkg->pkg_path = realpath(path, NULL);

    struct package_metadata *meta =
        package_read_metadata(pkg->pkg_path ? pkg->pkg_path : path);
    if (!meta)
    {
        package_free(pkg);
        return NULL;
    }

    package_metadata_free(pkg->meta);
    pkg->meta = meta;
    return pkg;
}
