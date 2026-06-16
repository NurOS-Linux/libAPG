// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "../../include/apg/db.h"
#include "../../include/apg/package.h"

static bool
name_in_deps(const char *name, struct package **all, int all_count)
{
    for (int i = 0; i < all_count; i++)
    {
        if (!all[i]->meta)
            continue;
        const struct dep_constraint_list *deps = &all[i]->meta->dependencies;
        for (int j = 0; j < deps->count; j++)
            if (deps->items[j].name && strcmp(deps->items[j].name, name) == 0)
                return true;
    }
    return false;
}

static bool
is_needed(struct package *pkg, struct package **all, int all_count)
{
    if (!pkg->meta || !pkg->meta->name)
        return false;
    if (name_in_deps(pkg->meta->name, all, all_count))
        return true;
    const struct str_list *prov = &pkg->meta->provides;
    for (int i = 0; i < prov->count; i++)
        if (prov->items[i] && name_in_deps(prov->items[i], all, all_count))
            return true;
    return false;
}

char **
db_get_orphans(struct db_handle *db, int *count)
{
    *count = 0;
    if (!db)
        return NULL;

    int all_count = 0;
    struct package **all = db_list(db, &all_count);
    if (!all)
        return NULL;

    int cap = 8;
    char **result = malloc(cap * sizeof(char *));
    if (!result)
        goto out;

    for (int i = 0; i < all_count; i++)
    {
        struct package *p = all[i];
        if (!p->meta || !p->meta->name)
            continue;
        if (p->installed_by_hand)
            continue;
        if (is_needed(p, all, all_count))
            continue;

        if (*count == cap)
        {
            cap *= 2;
            char **tmp = realloc(result, cap * sizeof(char *));
            if (!tmp)
                break;
            result = tmp;
        }
        char *name = strdup(p->meta->name);
        if (!name)
            break;
        result[(*count)++] = name;
    }

out:
    for (int i = 0; i < all_count; i++)
        package_free(all[i]);
    free(all);
    return result;
}
