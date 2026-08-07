// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../include/apg/graph.h"

#define GRAPH_INITIAL_CAP 16

struct dep_node
{
    char *name;                         // owned copy
    const struct package_metadata *pkg; // not owned
    bool installed;
};

struct alias_entry
{
    char *alias; // owned copy (from provides/replaces)
    size_t node_idx;
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
    struct alias_entry *aliases;
    size_t alias_count;
    size_t alias_cap;
    struct dep_provider_pref *prefs;
    size_t pref_count;
    size_t pref_cap;
};


size_t dep_graph_find(const struct dep_graph *g, const char *name);

size_t dep_graph_lookup(const struct dep_graph *g, const char *name);

dep_error_t dep_graph_prefer(struct dep_graph *g, const char *alias,
                             const char *pkg_name);
