// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "../../include/apg/db.h"
#include "../../include/apg/package.h"
#include "../hashmap_priv.h"
#include "db_priv.h"

static bool
build_needed_set(struct str_map *needed, struct package **all, int all_count)
{
    for (int i = 0; i < all_count; i++)
    {
        if (!all[i]->meta)
            continue;
        const struct dep_constraint_list *deps = &all[i]->meta->dependencies;
        for (int j = 0; j < deps->count; j++)
            if (deps->items[j].name &&
                !str_map_set(needed, deps->items[j].name, 1))
                return false;
    }
    return true;
}

static bool
is_needed(struct package *pkg, const struct str_map *needed)
{
    if (!pkg->meta || !pkg->meta->name)
        return false;
    size_t unused;
    if (str_map_get(needed, pkg->meta->name, &unused))
        return true;
    const struct str_list *prov = &pkg->meta->provides;
    for (int i = 0; i < prov->count; i++)
        if (prov->items[i] && str_map_get(needed, prov->items[i], &unused))
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

    struct str_map needed = {0};
    if (!str_map_init(&needed) || !build_needed_set(&needed, all, all_count))
    {
        str_map_free(&needed);
        for (int i = 0; i < all_count; i++)
            package_free(all[i]);
        free(all);
        return NULL;
    }

    struct str_vec result = {0};

    for (int i = 0; i < all_count; i++)
    {
        struct package *p = all[i];
        if (!p->meta || !p->meta->name)
            continue;
        if (p->installed_by_hand)
            continue;
        if (is_needed(p, &needed))
            continue;

        if (!str_vec_push(&result, p->meta->name))
            break;
    }

    str_map_free(&needed);
    for (int i = 0; i < all_count; i++)
        package_free(all[i]);
    free(all);
    *count = result.count;
    return result.items;
}

static bool
provides_name(const struct package *p, const char *name)
{
    if (strcmp(p->meta->name, name) == 0)
        return true;
    for (int i = 0; i < p->meta->provides.count; i++)
        if (strcmp(p->meta->provides.items[i], name) == 0)
            return true;
    return false;
}

char **
db_get_orphans_transitive(struct db_handle *db, int *count)
{
    *count = 0;
    if (!db)
        return NULL;

    int all_count = 0;
    struct package **all = db_list(db, &all_count);
    if (!all)
        return NULL;

    bool *reached = calloc((size_t)all_count + 1, sizeof(*reached));
    int *queue = malloc(((size_t)all_count + 1) * sizeof(*queue));
    struct str_vec result = {0};
    if (!reached || !queue)
        goto done;

    int head = 0;
    int tail = 0;
    for (int i = 0; i < all_count; i++)
    {
        struct package *p = all[i];
        if (!p->meta || !p->meta->name)
            continue;
        if (p->installed_by_hand)
        {
            reached[i] = true;
            queue[tail++] = i;
        }
    }

    while (head < tail)
    {
        const struct dep_constraint_list *deps =
            &all[queue[head++]]->meta->dependencies;
        for (int d = 0; d < deps->count; d++)
        {
            for (int i = 0; i < all_count; i++)
            {
                struct package *p = all[i];
                if (reached[i] || !p->meta || !p->meta->name)
                    continue;
                if (!provides_name(p, deps->items[d].name))
                    continue;
                reached[i] = true;
                queue[tail++] = i;
            }
        }
    }

    for (int i = 0; i < all_count; i++)
    {
        struct package *p = all[i];
        if (reached[i] || !p->meta || !p->meta->name)
            continue;
        if (!str_vec_push(&result, p->meta->name))
            break;
    }

done:
    free(reached);
    free(queue);
    for (int i = 0; i < all_count; i++)
        package_free(all[i]);
    free(all);
    *count = result.count;
    return result.items;
}
