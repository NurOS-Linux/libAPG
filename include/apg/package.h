// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2025 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#ifndef APG_PACKAGE_H
#define APG_PACKAGE_H

/**
 * @file package.h
 * @brief Core package types and lifecycle functions.
 */

#include <stdbool.h>
#include "version.h"

/**
 * @brief A list of heap-allocated strings.
 *
 * Not NULL-terminated. Ownership belongs to the containing structure,
 * typically @ref package_metadata. Free with str_list_free().
 */
struct str_list {
    char **items; /**< Array of string pointers. */
    int count;    /**< Number of elements in @p items. */
};

/**
 * @brief Metadata describing a package.
 *
 * All string fields are heap-allocated and owned by this structure.
 * Free with package_metadata_free().
 */
struct package_metadata {
    char *name;          /**< Package name. */
    char *version;       /**< Package version string. */
    char *type;          /**< Package type identifier. */
    char *architecture;  /**< Target architecture. */
    char *description;   /**< Short human-readable description. */
    char *maintainer;    /**< Maintainer contact string. */
    char *license;       /**< SPDX license identifier. */
    char *homepage;      /**< Upstream homepage URL. */
    struct str_list tags;                    /**< Classification tags. */
    struct dep_constraint_list dependencies; /**< Required dependencies. */
    struct str_list conflicts;               /**< Packages that conflict with this one. */
    struct str_list provides;                /**< Virtual package names provided. */
    struct str_list replaces;                /**< Packages superseded by this one. */
    struct str_list conf;                    /**< Configuration file paths owned by this package. */
};

/**
 * @brief A fully resolved package ready for installation.
 *
 * Free with package_free().
 */
struct package {
    struct package_metadata *meta; /**< Owned metadata. Never NULL on a valid package. */
    char *pkg_path;                /**< Filesystem path to the package archive. */
    struct str_list package_files; /**< Files contained in the archive. */
    bool installed_by_hand;        /**< True when explicitly requested by the user. */
};

/**
 * @brief Release all memory owned by a str_list.
 *
 * Does not free @p list itself.
 *
 * @param list List to clear. May be NULL.
 */
void str_list_free(struct str_list *list);

/**
 * @brief Release a package_metadata and all its fields.
 *
 * @param meta Metadata to free. May be NULL.
 */
void package_metadata_free(struct package_metadata *meta);

/**
 * @brief Release a package and all its owned resources.
 *
 * @param pkg Package to free. May be NULL.
 */
void package_free(struct package *pkg);

/**
 * @brief Allocate and zero-initialise a new package.
 *
 * @return Heap-allocated package, or NULL on allocation failure.
 */
struct package *package_new(void);

/**
 * @brief Allocate and zero-initialise a new package_metadata.
 *
 * @return Heap-allocated metadata, or NULL on allocation failure.
 */
struct package_metadata *package_metadata_new(void);

/**
 * @brief Install a package into the live filesystem root.
 *
 * Convenience wrapper around install_package_in_root() with @c root_path
 * set to @c "/".
 *
 * @param pkg Package to install.
 * @return true on success, false on any error.
 */
bool install_package(struct package *pkg);

/**
 * @brief Install a package into an alternative filesystem root.
 *
 * @param pkg       Package to install.
 * @param root_path Filesystem root for installation (e.g. @c "/mnt").
 * @return true on success, false on any error.
 */
bool install_package_in_root(struct package *pkg, const char *root_path);

/**
 * @brief Parse a package from disk.
 *
 * @param path      Path to the package archive.
 * @param root_path Filesystem root used to resolve relative paths.
 * @return Heap-allocated package on success, NULL on parse failure.
 *         Caller must call package_free() when done.
 */
struct package *parse_package(const char *path, const char *root_path);

#endif
