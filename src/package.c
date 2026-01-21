// NurOS Ruzen42 2026 apg/package.c
// Last change: Jan 11 

#include <stdlib.h>

#include "../include/apg/archive.h"
#include "../include/apg/package.h"
#include "../include/util.h"

struct package_metadata *
package_metadata_new(void)
{
    struct package_metadata *meta;
    meta = calloc(1, sizeof(*meta));
    if (!meta) return NULL;
    return meta;
}

struct package *
package_new(void)
{
    struct package *pkg;
    pkg = calloc(1, sizeof(*pkg));
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
    free((void*)pkg->pkg_path);
    str_list_free(&pkg->package_files);
    free(pkg);
}

void
install_package(struct package *pkg, char *path)
{
    log_two(INF, "Installing package into: ", path, stdout);


}
