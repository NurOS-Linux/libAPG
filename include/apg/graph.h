// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "package.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    DEP_OK = 0,
    DEP_ERR_NOMEM,
    DEP_ERR_CYCLE,
    DEP_ERR_MISSING,
    DEP_ERR_CONFLICT,
    DEP_ERR_VERSION,
} dep_error_t;

struct dep_graph;

struct dep_graph *dep_graph_new(void);
void dep_graph_free(struct dep_graph *g);

// Add a package to the available pool. Duplicate names are silently ignored.
dep_error_t dep_graph_add(struct dep_graph *g, const struct package_metadata *pkg);

// Resolve transitive install order for pkg_name via topological sort (DFS).
// *order is a malloc'd array of name pointers borrowed from the graph.
// Caller must free(*order) but not the individual strings.
dep_error_t dep_graph_resolve(struct dep_graph *g, const char *pkg_name,
                               char ***order, size_t *count);

// Returns true if any circular dependency exists in the graph.
bool dep_graph_has_cycle(struct dep_graph *g);

// Find which packages from installed[] would conflict if pkg_name is installed.
// *breaks is a malloc'd array of pointers borrowed from installed[]; caller frees.
dep_error_t dep_graph_find_breaks(struct dep_graph *g,
                                   const char *pkg_name,
                                   const char **installed,
                                   size_t installed_count,
                                   char ***breaks,
                                   size_t *break_count);
