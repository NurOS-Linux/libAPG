// NurOS Ruzen42 2026 apg/package.c
// Last change: Feb 5

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/apg/package.h"
#include "../include/util.h"
#include "../include/apg/archive.h"
#include "apg/json.h"

static const char *tmp_path = "/tmp/apg/";

struct package_metadata *
package_metadata_new(void)
{
    struct package_metadata *meta = calloc(1, sizeof(*meta));
    if (!meta) return NULL;
    return meta;
}

struct package *
package_new(void)
{
    // ReSharper disable once CppDFAMemoryLeak
    struct package *pkg = calloc(1, sizeof(*pkg));
    if (!pkg) return NULL;

    pkg->meta = package_metadata_new();
    if (!pkg->meta) {
        free(pkg);
        return NULL;
    }
    return pkg;
}

void
str_list_free(struct str_list *list)
{
    if (!list) return;
    free(list->items);
}

void
package_metadata_free(struct package_metadata *meta)
{
    if (!meta) return;

    free(meta->name);
    free(meta->version);
    free(meta->architecture);
    free(meta->maintainer);
    free(meta->license);
    free(meta->homepage);

    str_list_free(&meta->dependencies);
    str_list_free(&meta->conflicts);
    str_list_free(&meta->provides);
    str_list_free(&meta->replaces);
    str_list_free(&meta->tags);

    free(meta);
}

void
package_free(struct package *pkg)
{
    if (!pkg) return;
    package_metadata_free(pkg->meta);
    free((void *) pkg->pkg_path);
    str_list_free(&pkg->package_files);
    free(pkg);
}

bool
install_package(const struct package *pkg)
{
    return install_package_in_root(pkg, "/");
}

bool
install_package_in_root(const struct package *pkg, const char *root_path)
{
    log_two(INF, "Installing package into: ", root_path, stdout);

    // signing will be there
    char *real_tmp = concat_dirs(root_path, tmp_path);
    create_dir(real_tmp);
    free(real_tmp);

    return true;
}

struct package *
parse_package(const char *path, const char *root_path)
{
    // ReSharper disable once CppDFAMemoryLeak
    struct package *pkg = package_new();

    pkg->pkg_path = realpath(path, NULL);

    char *real_tmp = concat_dirs(root_path, path);
    if (!unarchive_package_in_root(pkg, real_tmp)) return NULL;
    free(real_tmp);

    package_metadata_from_file()



    return pkg;
}
