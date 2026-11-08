// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "db_priv.h"
#include "../../include/apg/db.h"
#include "../../include/apg/package.h"

static bool
provides_name(const struct package_metadata *meta, const char *name)
{
    if (strcmp(meta->name, name) == 0)
        return true;
    for (int i = 0; i < meta->provides.count; i++)
        if (strcmp(meta->provides.items[i], name) == 0)
            return true;
    return false;
}

char **
db_get_providers(struct db_handle *db, const char *name, int *count)
{
    *count = 0;
    if (!db || !name)
        return NULL;

    int all_count = 0;
    struct package **all = db_list(db, &all_count);
    if (!all)
        return NULL;

    struct str_vec result = {0};
    for (int i = 0; i < all_count; i++)
    {
        const struct package_metadata *meta = all[i]->meta;
        if (meta && meta->name && provides_name(meta, name) &&
            !str_vec_push(&result, meta->name))
            break;
    }

    for (int i = 0; i < all_count; i++)
        package_free(all[i]);
    free(all);
    *count = result.count;
    return result.items;
}
