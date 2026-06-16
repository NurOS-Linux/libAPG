// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file db.h
 * @brief Package database: open, query, and modify the installed-package store.
 */

#include <stdbool.h>
#include "package.h"

/**
 * @brief Operation type passed to database hook callbacks.
 */
typedef enum
{
    DB_OP_ADD,    /**< A package is being added to the database. */
    DB_OP_REMOVE, /**< A package is being removed from the database. */
} db_op_t;

/**
 * @brief Callback signature for pre/post write hooks.
 *
 * @param op       The operation being performed.
 * @param pkg_name Name of the package being added or removed.
 * @param userdata Caller-supplied context pointer from @ref db_hooks.
 */
typedef void (*db_hook_fn)(db_op_t op, const char *pkg_name, void *userdata);

/**
 * @brief Pre and post write callbacks registered on a database handle.
 */
struct db_hooks
{
    db_hook_fn pre;  /**< Called before each write operation. May be NULL. */
    db_hook_fn post; /**< Called after each write operation. May be NULL. */
    void *userdata;  /**< Opaque pointer forwarded to both callbacks. */
};

/**
 * @brief Opaque handle representing an open package database.
 */
struct db_handle;

/**
 * @brief Open a package database for reading and writing.
 *
 * @param path Filesystem path to the database directory.
 * @return Heap-allocated handle, or NULL on failure.
 *         Close with db_close() when done.
 */
struct db_handle *db_open(const char *path);

/**
 * @brief Open a package database in read-only mode.
 *
 * db_add() and db_remove() will return false on a read-only handle.
 *
 * @param path Filesystem path to the database directory.
 * @return Heap-allocated handle, or NULL on failure.
 *         Close with db_close() when done.
 */
struct db_handle *db_open_readonly(const char *path);

/**
 * @brief Close a database handle and release all associated resources.
 *
 * @param db Handle to close. May be NULL.
 */
void db_close(struct db_handle *db);

/**
 * @brief Register pre/post callbacks invoked around every write operation.
 *
 * Replaces any previously registered hooks. Pass @c NULL to clear hooks.
 *
 * @param db    Database handle.
 * @param hooks Hook structure to copy. May be NULL to clear hooks.
 */
void db_set_hooks(struct db_handle *db, const struct db_hooks *hooks);

/**
 * @brief Add or update a package record in the database.
 *
 * @param db  Database handle opened for writing.
 * @param pkg Package to record. The database stores a serialized copy.
 * @return true on success, false on write failure or read-only handle.
 */
bool db_add(struct db_handle *db, struct package *pkg);

/**
 * @brief Remove a package record from the database.
 *
 * @param db       Database handle opened for writing.
 * @param pkg_name Name of the package to remove.
 * @return true on success, false if the package was not found or the handle
 *         is read-only.
 */
bool db_remove(struct db_handle *db, const char *pkg_name);

/**
 * @brief Look up a package record by name.
 *
 * @param db   Database handle.
 * @param name Package name to look up.
 * @return Heap-allocated package on success, NULL if not found.
 *         Caller must call package_free() when done.
 */
struct package *db_get(struct db_handle *db, const char *name);

/**
 * @brief List all packages in the database.
 *
 * @param db    Database handle.
 * @param count Output parameter set to the number of returned packages.
 * @return Heap-allocated NULL-terminated array of heap-allocated packages, or
 *         NULL on failure. Caller must free each element and the array itself.
 */
struct package **db_list(struct db_handle *db, int *count);

/**
 * @brief Find the package that owns a given filesystem path.
 *
 * @param db   Database handle.
 * @param path Absolute or relative file path to look up.
 * @return Heap-allocated package name string on success, NULL if no package
 *         owns @p path. Caller must free() the returned string.
 */
char *db_owner(struct db_handle *db, const char *path);

/**
 * @brief Aggregate statistics about the installed package database.
 */
struct db_stats
{
    int package_count; /**< Number of installed packages. */
    int file_count;    /**< Total number of files across all packages. */
};

/**
 * @brief Populate a @ref db_stats structure with current database statistics.
 *
 * @p package_count is derived in O(1) from the LMDB page tree. @p file_count
 * requires a full scan of the file index.
 *
 * @param db  Database handle.
 * @param out Output structure to populate.
 * @return true on success, false if the database could not be queried.
 */
bool db_stats(struct db_handle *db, struct db_stats *out);

/**
 * @brief Find installed packages that were auto-installed and are no longer
 *        required by any other installed package.
 *
 * A package is considered an orphan when its @c installed_by_hand field is
 * false and no other installed package lists its name (or any of its
 * @c provides entries) as a dependency.
 *
 * @param db    Database handle.
 * @param count Output parameter set to the number of orphans found.
 * @return Heap-allocated array of heap-allocated package name strings, or NULL
 *         on failure. Caller must free() each element and the array itself.
 */
char **db_get_orphans(struct db_handle *db, int *count);

/**
 * @brief Search installed packages by name or description substring.
 *
 * Matching is case-insensitive. An empty @p query returns NULL.
 *
 * @param db    Database handle.
 * @param query Substring to search for.
 * @param count Output parameter set to the number of matching packages.
 * @return Heap-allocated array of heap-allocated packages, or NULL on failure.
 *         Caller must call package_free() on each element and free() the array.
 */
struct package **db_search(struct db_handle *db, const char *query, int *count);

/**
 * @brief Find all installed packages that depend on a given package.
 *
 * Checks direct dependencies and virtual names listed in the target package's
 * @c provides field, so removing a provider is flagged even when dependents
 * name the virtual package rather than the real one.
 *
 * @param db       Database handle.
 * @param pkg_name Name of the package being queried.
 * @param count    Output parameter set to the number of dependents found.
 * @return Heap-allocated array of heap-allocated package name strings, or NULL
 *         on failure. Caller must free() each element and the array itself.
 *         @p *count is set to 0 when no dependents are found.
 */
char **db_get_dependents(struct db_handle *db, const char *pkg_name,
                         int *count);
