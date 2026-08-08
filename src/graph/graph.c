// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "graph_priv.h"

struct dep_graph *
dep_graph_new(void)
{
    struct dep_graph *g = calloc(1, sizeof(*g));
    if (!g)
        return NULL;

    g->nodes = malloc(GRAPH_INITIAL_CAP * sizeof(*g->nodes));
    if (!g->nodes)
        goto fail;
    g->cap = GRAPH_INITIAL_CAP;

    if (!str_map_init(&g->node_map))
        goto fail;
    if (!str_map_init(&g->alias_map))
        goto fail;

    return g;

fail:
    free(g->nodes);
    str_map_free(&g->node_map);
    str_map_free(&g->alias_map);
    free(g);
    return NULL;
}

void
dep_graph_free(struct dep_graph *g)
{
    if (!g)
        return;

    for (size_t i = 0; i < g->count; i++)
    {
        free(g->nodes[i]->name);
        free(g->nodes[i]);
    }
    free(g->nodes);
    str_map_free(&g->node_map);

    for (size_t i = 0; i < g->alias_group_count; i++)
    {
        free(g->alias_groups[i].alias);
        free(g->alias_groups[i].providers);
    }
    free(g->alias_groups);
    str_map_free(&g->alias_map);

    for (size_t i = 0; i < g->pref_count; i++)
    {
        free(g->prefs[i].alias);
        free(g->prefs[i].pkg_name);
    }
    free(g->prefs);

    free(g);
}

size_t
dep_graph_find(const struct dep_graph *g, const char *name)
{
    size_t idx;
    if (str_map_get(&g->node_map, name, &idx))
        return idx;
    return SIZE_MAX;
}

size_t
dep_graph_lookup(const struct dep_graph *g, const char *name)
{
    size_t idx = dep_graph_find(g, name);
    if (idx != SIZE_MAX)
        return idx;

    for (size_t i = 0; i < g->pref_count; i++)
    {
        if (strcmp(g->prefs[i].alias, name) != 0)
            continue;
        size_t pref_idx = dep_graph_find(g, g->prefs[i].pkg_name);
        if (pref_idx != SIZE_MAX)
            return pref_idx;
        break;
    }

    size_t group_idx;
    if (!str_map_get(&g->alias_map, name, &group_idx))
        return SIZE_MAX;

    const struct alias_group *group = &g->alias_groups[group_idx];
    size_t first_match = SIZE_MAX;
    for (size_t i = 0; i < group->provider_count; i++)
    {
        size_t node_idx = group->providers[i];
        if (first_match == SIZE_MAX)
            first_match = node_idx;
        if (g->nodes[node_idx]->installed)
            return node_idx;
    }
    return first_match;
}

static dep_error_t
add_alias(struct dep_graph *g, const char *alias, size_t node_idx)
{
    size_t group_idx;
    if (!str_map_get(&g->alias_map, alias, &group_idx))
    {
        if (g->alias_group_count == g->alias_group_cap)
        {
            size_t new_cap = g->alias_group_cap == 0 ? GRAPH_INITIAL_CAP
                                                     : g->alias_group_cap * 2;
            struct alias_group *tmp =
                realloc(g->alias_groups, new_cap * sizeof(*tmp));
            if (!tmp)
                return DEP_ERR_NOMEM;
            g->alias_groups = tmp;
            g->alias_group_cap = new_cap;
        }

        char *dup = strdup(alias);
        if (!dup)
            return DEP_ERR_NOMEM;

        group_idx = g->alias_group_count;
        g->alias_groups[group_idx] = (struct alias_group){0};
        g->alias_groups[group_idx].alias = dup;
        g->alias_group_count++;

        if (!str_map_set(&g->alias_map, alias, group_idx))
        {
            free(g->alias_groups[group_idx].alias);
            g->alias_group_count--;
            return DEP_ERR_NOMEM;
        }
    }

    struct alias_group *group = &g->alias_groups[group_idx];
    if (group->provider_count == group->provider_cap)
    {
        size_t new_cap = group->provider_cap == 0 ? 2 : group->provider_cap * 2;
        size_t *tmp = realloc(group->providers, new_cap * sizeof(*tmp));
        if (!tmp)
            return DEP_ERR_NOMEM;
        group->providers = tmp;
        group->provider_cap = new_cap;
    }
    group->providers[group->provider_count++] = node_idx;
    return DEP_OK;
}

dep_error_t
dep_graph_prefer(struct dep_graph *g, const char *alias, const char *pkg_name)
{
    if (!g || !alias || !pkg_name)
        return DEP_ERR_NOMEM;

    for (size_t i = 0; i < g->pref_count; i++)
    {
        if (strcmp(g->prefs[i].alias, alias) != 0)
            continue;
        char *dup = strdup(pkg_name);
        if (!dup)
            return DEP_ERR_NOMEM;
        free(g->prefs[i].pkg_name);
        g->prefs[i].pkg_name = dup;
        return DEP_OK;
    }

    if (g->pref_count == g->pref_cap)
    {
        size_t new_cap = g->pref_cap == 0 ? GRAPH_INITIAL_CAP : g->pref_cap * 2;
        struct dep_provider_pref *tmp =
            realloc(g->prefs, new_cap * sizeof(*tmp));
        if (!tmp)
            return DEP_ERR_NOMEM;
        g->prefs = tmp;
        g->pref_cap = new_cap;
    }

    char *alias_dup = strdup(alias);
    char *pkg_dup = strdup(pkg_name);
    if (!alias_dup || !pkg_dup)
    {
        free(alias_dup);
        free(pkg_dup);
        return DEP_ERR_NOMEM;
    }
    g->prefs[g->pref_count].alias = alias_dup;
    g->prefs[g->pref_count].pkg_name = pkg_dup;
    g->pref_count++;
    return DEP_OK;
}

static dep_error_t
add_node(struct dep_graph *g, const struct package_metadata *pkg,
         bool installed)
{
    if (!g || !pkg || !pkg->name)
        return DEP_ERR_NOMEM;

    size_t existing_idx = dep_graph_find(g, pkg->name);
    if (existing_idx != SIZE_MAX)
    {
        if (installed)
            g->nodes[existing_idx]->installed = true;
        return DEP_OK;
    }

    if (g->count == g->cap)
    {
        size_t new_cap = g->cap == 0 ? GRAPH_INITIAL_CAP : g->cap * 2;
        struct dep_node **tmp = realloc(g->nodes, new_cap * sizeof(*tmp));
        if (!tmp)
            return DEP_ERR_NOMEM;
        g->nodes = tmp;
        g->cap = new_cap;
    }

    struct dep_node *node = malloc(sizeof(*node));
    if (!node)
        return DEP_ERR_NOMEM;
    node->name = strdup(pkg->name);
    if (!node->name)
    {
        free(node);
        return DEP_ERR_NOMEM;
    }
    node->pkg = pkg;
    node->installed = installed;

    size_t idx = g->count;
    g->nodes[idx] = node;
    g->count++;

    if (!str_map_set(&g->node_map, node->name, idx))
    {
        g->count--;
        free(node->name);
        free(node);
        return DEP_ERR_NOMEM;
    }

    for (int i = 0; i < pkg->provides.count; i++)
    {
        dep_error_t err = add_alias(g, pkg->provides.items[i], idx);
        if (err != DEP_OK)
            return err;
    }
    for (int i = 0; i < pkg->replaces.count; i++)
    {
        // Only register a replaces alias if no real node has that name yet
        if (dep_graph_find(g, pkg->replaces.items[i]) == SIZE_MAX)
        {
            dep_error_t err = add_alias(g, pkg->replaces.items[i], idx);
            if (err != DEP_OK)
                return err;
        }
    }

    return DEP_OK;
}

dep_error_t
dep_graph_add(struct dep_graph *g, const struct package_metadata *pkg)
{
    return add_node(g, pkg, false);
}

dep_error_t
dep_graph_add_installed(struct dep_graph *g, const struct package_metadata *pkg)
{
    return add_node(g, pkg, true);
}
