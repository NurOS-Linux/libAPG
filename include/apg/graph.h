// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file graph.h
 * @brief Dependency graph: resolution, cycle detection, and conflict analysis.
 */

#include "package.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Error codes returned by dependency graph operations.
 */
typedef enum
{
    DEP_OK = 0,       /**< Success. */
    DEP_ERR_NOMEM,    /**< Memory allocation failure. */
    DEP_ERR_CYCLE,    /**< Circular dependency detected. */
    DEP_ERR_MISSING,  /**< Required package is not in the graph. */
    DEP_ERR_CONFLICT, /**< Package conflict detected. */
    DEP_ERR_VERSION,  /**< No available version satisfies the constraint. */
} dep_error_t;

/**
 * @brief Opaque dependency graph handle.
 */
struct dep_graph;

/**
 * @brief Allocate a new, empty dependency graph.
 *
 * @return Heap-allocated graph, or NULL on allocation failure.
 *         Free with dep_graph_free().
 */
struct dep_graph *dep_graph_new(void);

/**
 * @brief Free a dependency graph and all its owned resources.
 *
 * @param g Graph to free. May be NULL.
 */
void dep_graph_free(struct dep_graph *g);

/**
 * @brief Add a package to the available pool.
 *
 * Duplicate names are silently ignored; the existing entry is kept.
 *
 * @param g   Graph to modify.
 * @param pkg Package metadata to add. The graph borrows the pointer.
 * @return @ref DEP_OK, or @ref DEP_ERR_NOMEM on allocation failure.
 */
dep_error_t dep_graph_add(struct dep_graph *g,
                          const struct package_metadata *pkg);

/**
 * @brief Resolve the transitive install order for a package.
 *
 * Uses a depth-first topological sort. @p *order is set to a malloc'd array
 * of name pointers that are borrowed from the graph. The caller must free
 * @p *order but must not free the individual string pointers.
 *
 * @param g        Graph containing the available package pool.
 * @param pkg_name Name of the package to resolve.
 * @param order    Output: heap-allocated array of package names in install
 * order.
 * @param count    Output: number of entries in @p *order.
 * @return @ref DEP_OK on success, or an error code.
 */
dep_error_t dep_graph_resolve(struct dep_graph *g, const char *pkg_name,
                              char ***order, size_t *count);

/**
 * @brief Check whether the graph contains any circular dependency.
 *
 * @param g Graph to inspect.
 * @return true if at least one cycle exists.
 */
bool dep_graph_has_cycle(struct dep_graph *g);

/**
 * @brief Find which installed packages would break if a new package is
 * installed.
 *
 * @p *breaks is set to a malloc'd array of pointers borrowed from @p installed.
 * The caller must free @p *breaks but must not free the individual strings.
 *
 * @param g               Graph containing the available pool.
 * @param pkg_name        Package being considered for installation.
 * @param installed       Array of currently installed package names.
 * @param installed_count Number of entries in @p installed.
 * @param breaks          Output: array of conflicting installed package names.
 * @param break_count     Output: number of entries in @p *breaks.
 * @return @ref DEP_OK if no conflicts, @ref DEP_ERR_CONFLICT if at least one
 *         conflict is found, or another error code on failure.
 */
dep_error_t dep_graph_find_breaks(struct dep_graph *g, const char *pkg_name,
                                  const char **installed,
                                  size_t installed_count, char ***breaks,
                                  size_t *break_count);
