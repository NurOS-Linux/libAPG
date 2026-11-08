// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "../../include/apg/db.h"
#include "../../include/apg/package.h"
#include "../../include/apg/version.h"
#include "db_priv.h"

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

    struct str_vec result = {0};

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

        if (!str_vec_push(&result, p->meta->name))
            break;
    }

    for (int i = 0; i < all_count; i++)
        package_free(all[i]);
    free(all);
    package_free(target);
    *count = result.count;
    return result.items;
}

static bool
depends_on(const struct package *p, const char *name,
           const struct str_list *provides)
{
    const struct dep_constraint_list *deps = &p->meta->dependencies;
    for (int i = 0; i < deps->count; i++)
        if (names_match(deps->items[i].name, name, provides))
            return true;
    return false;
}

char **
db_get_dependents_transitive(struct db_handle *db, const char *pkg_name,
                             int *count)
{
    *count = 0;
    if (!db || !pkg_name)
        return NULL;

    int all_count = 0;
    struct package **all = db_list(db, &all_count);
    if (!all)
        return NULL;

    bool *seen = calloc((size_t)all_count + 1, sizeof(*seen));
    int *queue = malloc(((size_t)all_count + 1) * sizeof(*queue));
    struct str_vec result = {0};
    if (!seen || !queue)
        goto done;

    int head = 0;
    int tail = 0;
    const struct str_list *root_provides = NULL;
    for (int i = 0; i < all_count; i++)
    {
        if (all[i]->meta && all[i]->meta->name &&
            strcmp(all[i]->meta->name, pkg_name) == 0)
        {
            seen[i] = true;
            root_provides = &all[i]->meta->provides;
        }
    }

    static const struct str_list no_provides = {0};
    if (!root_provides)
        root_provides = &no_provides;

    const char *name = pkg_name;
    const struct str_list *provides = root_provides;
    for (;;)
    {
        for (int i = 0; i < all_count; i++)
        {
            struct package *p = all[i];
            if (seen[i] || !p->meta || !p->meta->name)
                continue;
            if (!depends_on(p, name, provides))
                continue;
            seen[i] = true;
            queue[tail++] = i;
            if (!str_vec_push(&result, p->meta->name))
                goto done;
        }
        if (head == tail)
            break;
        struct package *next = all[queue[head++]];
        name = next->meta->name;
        provides = &next->meta->provides;
    }

done:
    free(seen);
    free(queue);
    for (int i = 0; i < all_count; i++)
        package_free(all[i]);
    free(all);
    *count = result.count;
    return result.items;
}
