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
        goto fail_nodes;
    g->cap = GRAPH_INITIAL_CAP;

    g->aliases = malloc(GRAPH_INITIAL_CAP * sizeof(*g->aliases));
    if (!g->aliases)
        goto fail_aliases;
    g->alias_cap = GRAPH_INITIAL_CAP;

    return g;

fail_aliases:
    free(g->nodes);
fail_nodes:
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
    for (size_t i = 0; i < g->alias_count; i++)
        free(g->aliases[i].alias);
    free(g->aliases);
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
    for (size_t i = 0; i < g->count; i++)
        if (strcmp(g->nodes[i]->name, name) == 0)
            return i;
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

    size_t first_match = SIZE_MAX;
    for (size_t i = 0; i < g->alias_count; i++)
    {
        if (strcmp(g->aliases[i].alias, name) != 0)
            continue;
        size_t node_idx = g->aliases[i].node_idx;
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
    if (g->alias_count == g->alias_cap)
    {
        size_t new_cap =
            g->alias_cap == 0 ? GRAPH_INITIAL_CAP : g->alias_cap * 2;
        struct alias_entry *tmp = realloc(g->aliases, new_cap * sizeof(*tmp));
        if (!tmp)
            return DEP_ERR_NOMEM;
        g->aliases = tmp;
        g->alias_cap = new_cap;
    }
    char *dup = strdup(alias);
    if (!dup)
        return DEP_ERR_NOMEM;
    g->aliases[g->alias_count].alias = dup;
    g->aliases[g->alias_count].node_idx = node_idx;
    g->alias_count++;
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
    if (dep_graph_find(g, pkg->name) != SIZE_MAX)
        return DEP_OK;

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
