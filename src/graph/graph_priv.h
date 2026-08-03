// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stddef.h>
#include <stdint.h>
#include "../../include/apg/graph.h"

#define GRAPH_INITIAL_CAP 16

struct dep_node
{
    char *name;                         // owned copy
    const struct package_metadata *pkg; // not owned
};

struct alias_entry
{
    char *alias; // owned copy (from provides/replaces)
    size_t node_idx;
};

struct dep_graph
{
    struct dep_node **nodes;
    size_t count;
    size_t cap;
    struct alias_entry *aliases;
    size_t alias_count;
    size_t alias_cap;
};

// Look up by exact package name, returns SIZE_MAX if absent
size_t dep_graph_find(const struct dep_graph *g, const char *name);

// Look up by name or alias (provides/replaces), returns SIZE_MAX if absent
size_t dep_graph_lookup(const struct dep_graph *g, const char *name);
