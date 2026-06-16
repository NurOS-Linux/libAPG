// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/apg/package.h"
#include "../include/apg/version.h"
#include "../include/apg/install.h"
#include "../include/util.h"
#include "../include/apg/scripts.h"
#include "../include/apg/checksum.h"
#include "../include/apg/archive.h"
#include "../include/apg/json.h"
#include "../include/util.h"

static const char *tmp_path = APG_TMP_DIR "/";

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
    char *real_tmp = concat_dirs(root_path, tmp_path);
    if (!real_tmp)
        return false;
    create_dir(real_tmp);

    if (!unarchive_package_in_root(pkg, real_tmp))
    {
        free(real_tmp);
        return false;
    }

    if (!verify_checksums(real_tmp))
    {
        free(real_tmp);
        return false;
    }

    if (!run_script(real_tmp, "pre-install"))
    {
        free(real_tmp);
        return false;
    }

    if (!install_data_dir(real_tmp, root_path))
    {
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

    if (!run_script(real_tmp, "post-install"))
    {
        rollback_install(real_tmp, root_path);
        free(real_tmp);
        return false;
    }

    free(real_tmp);
    return true;
}

bool
package_collect_files(struct package *pkg, const char *root_path)
{
    if (!pkg || !pkg->pkg_path)
        return false;

    char *real_tmp = concat_dirs(root_path, tmp_path);
    if (!real_tmp)
        return false;
    create_dir(real_tmp);

    if (!unarchive_package_in_root(pkg, real_tmp))
    {
        free(real_tmp);
        return false;
    }

    char *data_src = concat_dirs(real_tmp, "data");
    free(real_tmp);
    if (!data_src)
        return false;

    int file_count = 0;
    char **files = collect_files(data_src, &file_count);
    free(data_src);

    str_list_free(&pkg->package_files);
    pkg->package_files.items = files;
    pkg->package_files.count = file_count;
    return true;
}

struct package *
parse_package(const char *path, const char *root_path)
{
    struct package *pkg = package_new();
    if (!pkg)
        return NULL;

    pkg->pkg_path = realpath(path, NULL);

    char *real_tmp = concat_dirs(root_path, tmp_path);
    create_dir(real_tmp);

    if (!unarchive_package_in_root(pkg, real_tmp))
    {
        free(real_tmp);
        package_free(pkg);
        return NULL;
    }

    char *meta_path = concat_dirs(real_tmp, "meta.json");
    free(real_tmp);

    package_metadata_free(pkg->meta);
    pkg->meta = package_metadata_from_file(meta_path);
    free(meta_path);

    if (!pkg->meta)
    {
        package_free(pkg);
        return NULL;
    }

    return pkg;
}
