// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hashmap_priv.h"
#include "../../include/apg/graph.h"

#define GRAPH_INITIAL_CAP 16

struct dep_node
{
    char *name;                         // owned copy
    const struct package_metadata *pkg; // not owned
    bool installed;
};

struct alias_group
{
    char *alias;       // owned copy (from provides/replaces)
    size_t *providers; // owned array of node indices
    size_t provider_count;
    size_t provider_cap;
};

struct dep_provider_pref
{
    char *alias;    // owned copy
    char *pkg_name; // owned copy
};

struct dep_graph
{
    struct dep_node **nodes;
    size_t count;
    size_t cap;
    struct str_map node_map; // name -> index into nodes

    struct alias_group *alias_groups;
    size_t alias_group_count;
    size_t alias_group_cap;
    struct str_map alias_map; // alias -> index into alias_groups

    struct dep_provider_pref *prefs;
    size_t pref_count;
    size_t pref_cap;
};

size_t dep_graph_find(const struct dep_graph *g, const char *name);

size_t dep_graph_lookup(const struct dep_graph *g, const char *name);

dep_error_t dep_graph_prefer(struct dep_graph *g, const char *alias,
                             const char *pkg_name);
