// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "graph_priv.h"

static bool
str_list_contains(const struct str_list *list, const char *str)
{
    for (int i = 0; i < list->count; i++)
        if (strcmp(list->items[i], str) == 0)
            return true;
    return false;
}

dep_error_t
dep_graph_find_breaks(struct dep_graph *g, const char *pkg_name,
                      const char **installed, size_t installed_count,
                      char ***breaks, size_t *break_count)
{
    if (!g || !pkg_name || !breaks || !break_count)
        return DEP_ERR_NOMEM;

    size_t pkg_idx = dep_graph_lookup(g, pkg_name);
    if (pkg_idx == SIZE_MAX)
        return DEP_ERR_MISSING;

    const struct package_metadata *pkg = g->nodes[pkg_idx]->pkg;

    char **result = malloc(installed_count * sizeof(*result));
    if (!result)
        return DEP_ERR_NOMEM;
    size_t result_count = 0;

    for (size_t i = 0; i < installed_count; i++)
    {
        const char *inst_name = installed[i];
        size_t inst_idx = dep_graph_lookup(g, inst_name);
        bool conflict = false;

        // New package declares a conflict against this installed package
        if (str_list_contains(&pkg->conflicts, inst_name))
            conflict = true;

        if (!conflict && inst_idx != SIZE_MAX)
        {
            const struct package_metadata *inst = g->nodes[inst_idx]->pkg;

            // Installed package declares a conflict against the new package
            if (str_list_contains(&inst->conflicts, pkg->name))
                conflict = true;

            // Installed package conflicts with something the new package
            // provides
            if (!conflict)
            {
                for (int j = 0; j < pkg->provides.count && !conflict; j++)
                    if (str_list_contains(&inst->conflicts,
                                          pkg->provides.items[j]))
                        conflict = true;
            }

            // New package conflicts with something the installed package
            // provides
            if (!conflict)
            {
                for (int j = 0; j < inst->provides.count && !conflict; j++)
                    if (str_list_contains(&pkg->conflicts,
                                          inst->provides.items[j]))
                        conflict = true;
            }
        }

        if (conflict)
            result[result_count++] = (char *)inst_name;
    }

    if (result_count == 0)
    {
        free(result);
        *breaks = NULL;
        *break_count = 0;
    }
    else
    {
        char **shrunk = realloc(result, result_count * sizeof(*shrunk));
        *breaks = shrunk ? shrunk : result;
        *break_count = result_count;
    }

    return DEP_OK;
}
