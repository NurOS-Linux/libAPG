// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "db_priv.h"
#include "../../include/apg/db.h"
#include "../../include/apg/package.h"

char *
db_owner(struct db_handle *db, const char *path)
{
    if (!db || !path) return NULL;

    int count = 0;
    struct package **pkgs = db_list(db, &count);
    if (!pkgs) return NULL;

    char *owner = NULL;
    for (int i = 0; i < count && !owner; i++) {
        struct package *pkg = pkgs[i];
        if (!pkg->meta || !pkg->meta->name) continue;
        for (int j = 0; j < pkg->package_files.count; j++) {
            if (pkg->package_files.items[j] &&
                strcmp(pkg->package_files.items[j], path) == 0) {
                owner = strdup(pkg->meta->name);
                break;
            }
        }
    }

    for (int i = 0; i < count; i++)
        package_free(pkgs[i]);
    free(pkgs);
    return owner;
}
