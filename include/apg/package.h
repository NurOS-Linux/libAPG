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
#include "export.h"
#include "version.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief A list of heap-allocated strings.
     *
     * Not NULL-terminated. Ownership belongs to the containing structure,
     * typically @ref package_metadata. Free with str_list_free().
     */
    struct str_list
    {
        char **items; /**< Array of string pointers. */
        int count;    /**< Number of elements in @p items. */
    };

    /**
     * @brief Metadata describing a package.
     *
     * All string fields are heap-allocated and owned by this structure.
     * Free with package_metadata_free().
     */
    struct package_metadata
    {
        char *name;           /**< Package name. */
        char *version;        /**< Package version string. */
        char *type;           /**< Package type identifier. */
        char *architecture;   /**< Target architecture. */
        char *description;    /**< Short human-readable description. */
        char *maintainer;     /**< Maintainer contact string. */
        char *license;        /**< SPDX license identifier. */
        char *homepage;       /**< Upstream homepage URL. */
        struct str_list tags; /**< Classification tags. */
        struct dep_constraint_list dependencies; /**< Required dependencies. */
        struct str_list conflicts; /**< Packages that conflict with this one. */
        struct str_list provides;  /**< Virtual package names provided. */
        struct str_list replaces;  /**< Packages superseded by this one. */
        struct str_list
            conf; /**< Configuration file paths owned by this package. */
    };

    /**
     * @brief A fully resolved package ready for installation.
     *
     * Free with package_free().
     */
    struct package
    {
        struct package_metadata
            *meta;      /**< Owned metadata. Never NULL on a valid package. */
        char *pkg_path; /**< Filesystem path to the package archive. */
        struct str_list package_files; /**< Files contained in the archive. */
        bool installed_by_hand; /**< True when explicitly requested by the user.
                                 */
        bool held; /**< True when upgrades and removal are blocked. */
    };

    /**
     * @brief Release all memory owned by a str_list.
     *
     * Does not free @p list itself.
     *
     * @param list List to clear. May be NULL.
     */
    APG_API void str_list_free(struct str_list *list);

    /**
     * @brief Release a package_metadata and all its fields.
     *
     * @param meta Metadata to free. May be NULL.
     */
    APG_API void package_metadata_free(struct package_metadata *meta);

    /**
     * @brief Release a package and all its owned resources.
     *
     * @param pkg Package to free. May be NULL.
     */
    APG_API void package_free(struct package *pkg);

    /**
     * @brief Allocate and zero-initialise a new package.
     *
     * @return Heap-allocated package, or NULL on allocation failure.
     */
    APG_API struct package *package_new(void);

    /**
     * @brief Allocate and zero-initialise a new package_metadata.
     *
     * @return Heap-allocated metadata, or NULL on allocation failure.
     */
    APG_API struct package_metadata *package_metadata_new(void);

    /**
     * @brief Install a package into the live filesystem root.
     *
     * Convenience wrapper around install_package_in_root() with @c root_path
     * set to @c "/".
     *
     * @param pkg Package to install.
     * @return true on success, false on any error.
     */
    APG_API bool install_package(struct package *pkg);

    /**
     * @brief Install a package into an alternative filesystem root.
     *
     * @param pkg       Package to install.
     * @param root_path Filesystem root for installation (e.g. @c "/mnt").
     * @return true on success, false on any error.
     */
    APG_API bool install_package_in_root(struct package *pkg,
                                         const char *root_path);

    /**
     * @brief Collect the file list of a package archive without installing
     *        any files.
     *
     * Populates @p pkg->package_files so callers can inspect which paths the
     * package would install before committing to a transaction. Safe to call
     * multiple times — existing file list is replaced.
     *
     * @param pkg       Package whose archive to inspect.
     * @param root_path Unused; the archive is read without extraction. Kept
     *                  for interface stability.
     * @return true on success, false if the archive could not be read.
     */
    APG_API bool package_collect_files(struct package *pkg,
                                       const char *root_path);

    /**
     * @brief Parse a package from disk.
     *
     * @param path      Path to the package archive.
     * @param root_path Unused; the archive is not extracted. Kept for
     *                  interface stability.
     * @return Heap-allocated package on success, NULL on parse failure.
     *         Caller must call package_free() when done.
     */
    APG_API struct package *parse_package(const char *path,
                                          const char *root_path);

    /**
     * @brief Read the metadata of a package archive without extracting it.
     *
     * Reads only the @c metadata.json entry. Nothing is written to disk. The
     * entry must be at most 1 MiB.
     *
     * @param archive_path Path to the package archive.
     * @return Heap-allocated metadata, or NULL if the archive cannot be read,
     *         has no @c metadata.json, or the file cannot be parsed;
     *         apg_last_error() describes the cause. Free with
     *         package_metadata_free().
     */
    APG_API struct package_metadata *
    package_read_metadata(const char *archive_path);

    /**
     * @brief List the files a package archive would install, without
     *        extracting it.
     *
     * Returns every entry under the archive's @c data/ tree that is not a
     * directory (regular files, symbolic links, hard links), as paths that
     * start with @c / and are relative to the install root, for example
     * @c /usr/bin/tool. These are the paths package_collect_files() and
     * install_package_in_root() record in @c package_files. The archive is
     * read once and nothing is written to disk. An entry whose path contains a
     * @c .. component makes the call fail.
     *
     * @param archive_path Path to the package archive.
     * @param files        Output: heap-allocated array of heap-allocated path
     *                     strings, NULL when the archive has no files. Caller
     *                     must free() each element and the array.
     * @param count        Output: number of paths.
     * @return true if the archive was read (also when it has no files), false
     *         on error, with apg_last_error() describing it.
     */
    APG_API bool package_list_files(const char *archive_path, char ***files,
                                    int *count);

#ifdef __cplusplus
}
#endif

#endif
