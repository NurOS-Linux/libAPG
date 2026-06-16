// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "../../include/apg/db.h"
#include "../../include/apg/package.h"
#include "../../include/apg/version.h"

static bool
names_match(const char *needle, const char *pkg_name,
            const struct str_list *provides)
{
    if (strcmp(needle, pkg_name) == 0)
        return true;
    for (int i = 0; i < provides->count; i++)
        if (strcmp(needle, provides->items[i]) == 0)
            return true;
    return false;
}

char **
db_get_dependents(struct db_handle *db, const char *pkg_name, int *count)
{
    *count = 0;
    if (!db || !pkg_name)
        return NULL;

    struct str_list provides = {0};
    struct package *target = db_get(db, pkg_name);
    if (target && target->meta)
        provides = target->meta->provides;

    int all_count = 0;
    struct package **all = db_list(db, &all_count);
    if (!all)
    {
        package_free(target);
        return NULL;
    }

    int cap = 8;
    char **result = malloc(cap * sizeof(char *));
    if (!result)
        goto out;

    for (int i = 0; i < all_count; i++)
    {
        struct package *p = all[i];
        if (!p->meta || !p->meta->name)
            continue;
        if (strcmp(p->meta->name, pkg_name) == 0)
            continue;

        bool found = false;
        const struct dep_constraint_list *deps = &p->meta->dependencies;
        for (int j = 0; j < deps->count && !found; j++)
            found = names_match(deps->items[j].name, pkg_name, &provides);

        if (!found)
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
    package_free(target);
    return result;
}
