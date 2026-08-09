// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "candidates_priv.h"

#define CANDIDATE_SET_INITIAL_CAP 16
#define CANDIDATE_GROUP_INITIAL_CAP 4

bool
candidate_set_init(struct candidate_set *cs)
{
    cs->groups = NULL;
    cs->count = 0;
    cs->cap = 0;
    return str_map_init(&cs->index);
}

void
candidate_set_free(struct candidate_set *cs)
{
    for (size_t i = 0; i < cs->count; i++)
    {
        free(cs->groups[i].name);
        free(cs->groups[i].items);
    }
    free(cs->groups);
    cs->groups = NULL;
    cs->count = 0;
    cs->cap = 0;
    str_map_free(&cs->index);
}

static struct candidate_group *
find_or_create_group(struct candidate_set *cs, const char *name)
{
    size_t idx;
    if (str_map_get(&cs->index, name, &idx))
        return &cs->groups[idx];

    if (cs->count == cs->cap)
    {
        size_t new_cap = cs->cap ? cs->cap * 2 : CANDIDATE_SET_INITIAL_CAP;
        struct candidate_group *tmp =
            realloc(cs->groups, new_cap * sizeof(*tmp));
        if (!tmp)
            return NULL;
        cs->groups = tmp;
        cs->cap = new_cap;
    }

    char *name_copy = strdup(name);
    if (!name_copy)
        return NULL;

    if (!str_map_set(&cs->index, name, cs->count))
    {
        free(name_copy);
        return NULL;
    }

    struct candidate_group *g = &cs->groups[cs->count];
    g->name = name_copy;
    g->items = NULL;
    g->count = 0;
    g->cap = 0;
    cs->count++;
    return g;
}

bool
candidate_set_add(struct candidate_set *cs, const char *name,
                  const struct package_metadata *pkg)
{
    struct candidate_group *g = find_or_create_group(cs, name);
    if (!g)
        return false;

    if (g->count == g->cap)
    {
        size_t new_cap = g->cap ? g->cap * 2 : CANDIDATE_GROUP_INITIAL_CAP;
        const struct package_metadata **tmp =
            realloc(g->items, new_cap * sizeof(*tmp));
        if (!tmp)
            return false;
        g->items = tmp;
        g->cap = new_cap;
    }

    g->items[g->count++] = pkg;
    return true;
}

bool
candidate_set_add_package(struct candidate_set *cs,
                          const struct package_metadata *pkg)
{
    if (!candidate_set_add(cs, pkg->name, pkg))
        return false;

    for (int i = 0; i < pkg->provides.count; i++)
        if (!candidate_set_add(cs, pkg->provides.items[i], pkg))
            return false;

    return true;
}

const struct candidate_group *
candidate_set_lookup(const struct candidate_set *cs, const char *name)
{
    size_t idx;
    if (!str_map_get(&cs->index, name, &idx))
        return NULL;
    return &cs->groups[idx];
}
