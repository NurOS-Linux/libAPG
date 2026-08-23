// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file graph.h
 * @brief Dependency graph: resolution, cycle detection, and conflict analysis.
 */

#include "export.h"
#include "package.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

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
    APG_API struct dep_graph *dep_graph_new(void);

    /**
     * @brief Free a dependency graph and all its owned resources.
     *
     * @param g Graph to free. May be NULL.
     */
    APG_API void dep_graph_free(struct dep_graph *g);

    /**
     * @brief Add a package to the available pool.
     *
     * Duplicate names keep the metadata of whichever call added the name
     * first (so an upgrade's new version, added before the installed
     * pool is scanned, is what dependents check against).
     *
     * @param g   Graph to modify.
     * @param pkg Package metadata to add. The graph borrows the pointer.
     * @return @ref DEP_OK, or @ref DEP_ERR_NOMEM on allocation failure.
     */
    APG_API dep_error_t dep_graph_add(struct dep_graph *g,
                                      const struct package_metadata *pkg);

    /**
     * @brief Same as dep_graph_add(), but marks the package as installed.
     *
     * If two packages provide the same virtual name, lookups prefer the
     * installed one; with no installed provider, the first one added wins.
     * If @p pkg's name was already added (e.g. its upgrade target), this
     * only marks the existing node installed — an in-place upgrade still
     * counts as installed for that purpose.
     *
     * @param g   Graph to modify.
     * @param pkg Package metadata to add. The graph borrows the pointer.
     * @return @ref DEP_OK, or @ref DEP_ERR_NOMEM on allocation failure.
     */
    APG_API dep_error_t dep_graph_add_installed(
        struct dep_graph *g, const struct package_metadata *pkg);

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
    APG_API dep_error_t dep_graph_resolve(struct dep_graph *g,
                                          const char *pkg_name, char ***order,
                                          size_t *count);

    /**
     * @brief Resolve the transitive install order for multiple packages in
     * parallel.
     *
     * Resolves independent dependency graph branches concurrently using
     * threads, and merges them into a deduplicated topological install order.
     *
     * @param g          Graph containing the available package pool.
     * @param pkg_names  Array of package names to resolve.
     * @param count      Number of entries in @p pkg_names.
     * @param order      Output: heap-allocated array of package names in
     * install order.
     * @param order_count Output: number of entries in @p *order.
     * @return @ref DEP_OK on success, or an error code.
     */
    APG_API dep_error_t dep_graph_resolve_parallel(const struct dep_graph *g,
                                                   const char **pkg_names,
                                                   size_t count, char ***order,
                                                   size_t *order_count);

    /**
     * @brief Check whether the graph contains any circular dependency.
     *
     * @param g Graph to inspect.
     * @return true if at least one cycle exists.
     */
    APG_API bool dep_graph_has_cycle(struct dep_graph *g);

    /**
     * @brief Find which installed packages would break if a new package is
     * installed.
     *
     * @p *breaks is set to a malloc'd array of pointers borrowed from @p
     * installed. The caller must free @p *breaks but must not free the
     * individual strings.
     *
     * @param g               Graph containing the available pool.
     * @param pkg_name        Package being considered for installation.
     * @param installed       Array of currently installed package names.
     * @param installed_count Number of entries in @p installed.
     * @param breaks          Output: array of conflicting installed package
     * names.
     * @param break_count     Output: number of entries in @p *breaks.
     * @return @ref DEP_OK if no conflicts, @ref DEP_ERR_CONFLICT if at least
     * one conflict is found, or another error code on failure.
     */
    APG_API dep_error_t dep_graph_find_breaks(
        struct dep_graph *g, const char *pkg_name, const char **installed,
        size_t installed_count, char ***breaks, size_t *break_count);

    /**
     * @brief Export the graph to Graphviz DOT format.
     *
     * Every package is a node (installed ones filled green), every
     * dependency is an edge to whatever dep_graph_lookup() currently
     * resolves it to (labeled with the version constraint, if any; drawn
     * red and dashed if unresolved), and every virtual/provides name is a
     * diamond node with an edge to each of its providers — bold for the
     * one dep_graph_lookup() would pick, dashed for the rest.
     *
     * @param g Graph to export.
     * @return Heap-allocated DOT source; caller must free(). NULL on
     *         allocation failure.
     */
    APG_API char *dep_graph_export_dot(const struct dep_graph *g);

    /**
     * @brief Outcome of a dep_graph_resolve_sat() call.
     */
    typedef enum
    {
        SAT_SOLVE_SATISFIABLE,     /**< A consistent selection was found. */
        SAT_SOLVE_UNSATISFIABLE,   /**< No consistent selection exists. */
        SAT_SOLVE_BUDGET_EXCEEDED, /**< Search exhausted its decision budget,
                                        without a definitive answer. */
        SAT_SOLVE_ERROR,           /**< Allocation or internal failure. */
    } sat_solve_result_t;

    /**
     * @brief Resolve a set of required root packages against a pool of
     * candidates using a SAT solver.
     *
     * Unlike dep_graph_resolve(), which assumes exactly one candidate per
     * package name, this considers every package in @p candidates a
     * possible provider of its own name and its `provides` names, and
     * searches for a selection that satisfies every root's dependencies
     * and every selected package's conflicts simultaneously — including
     * conflicts between two different roots, which dep_graph_resolve()
     * cannot detect since it resolves one root at a time.
     *
     * Backed by a plain DPLL solver (no clause learning): correctness is
     * not affected, but pathological inputs can exhaust @p decision_budget
     * before finding an answer.
     *
     * @param candidates          Pool of packages that may be selected.
     *                            @p roots must be a subset of this array.
     * @param candidate_count     Number of entries in @p candidates.
     * @param roots               Packages that must be selected.
     * @param root_count          Number of entries in @p roots.
     * @param decision_budget     Maximum number of solver decisions before
     *                            giving up (@ref SAT_SOLVE_BUDGET_EXCEEDED).
     * @param thread_count        Number of parallel search strategies to
     *                            run; 1 disables parallelism.
     * @param out_selected        Set to a heap-allocated array of pointers
     *                            borrowed from @p candidates on
     *                            @ref SAT_SOLVE_SATISFIABLE, NULL otherwise.
     *                            Caller frees the array, not the pointed-to
     *                            packages.
     * @param out_selected_count  Set to the number of entries in
     *                            @p *out_selected.
     * @param out_conflict        If non-NULL, set to a heap-allocated
     *                            human-readable explanation on
     *                            @ref SAT_SOLVE_UNSATISFIABLE, NULL
     *                            otherwise. Caller frees it.
     * @return The solve outcome.
     */
    APG_API sat_solve_result_t dep_graph_resolve_sat(
        const struct package_metadata **candidates, size_t candidate_count,
        const struct package_metadata **roots, size_t root_count,
        size_t decision_budget, int thread_count,
        struct package_metadata ***out_selected, size_t *out_selected_count,
        char **out_conflict);

#ifdef __cplusplus
}
#endif
