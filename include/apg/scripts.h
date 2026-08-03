// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file scripts.h
 * @brief Pre/post-install script execution.
 */

#include "export.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Run a lifecycle script from a package's @c scripts/ directory.
     *
     * Executes @c pkg_dir/scripts/name if the file exists and is executable.
     *
     * If @p root_path is provided and is not @c "/" (or @c NULL), the child
     * process calls @c chroot(root_path) and @c chdir("/") before applying
     * sandbox primitives and executing the script. If @c chroot fails (e.g.
     * missing privileges), the call fails closed and returns false without
     * executing the script.
     *
     * On Linux, the script runs in a child process with isolated network,
     * mount, UTS, and IPC namespaces (@c unshare()). On FreeBSD, the script
     * runs under Capsicum capability mode (@c cap_enter()).
     *
     * Common script names are @c pre-install, @c post-install, @c pre-remove,
     * and @c post-remove.
     *
     * @param pkg_dir   Path to the extracted package directory.
     * @param name      Script filename (without path) to execute.
     * @param root_path Target filesystem root path, or NULL/"/" for live root.
     * @return true if the script succeeded or does not exist.
     *         false if the script exists but exited with a non-zero status,
     *         or if the chroot/sandbox setup failed.
     */
    APG_API bool run_script(const char *pkg_dir, const char *name,
                            const char *root_path);

    /**
     * @brief Compute the persistent storage path for a package's scripts/
     *        directory.
     *
     * This is the path used by scripts_persist() and scripts_persist_remove(),
     * and the @p pkg_dir to pass to run_script() when running @c pre-remove or
     * @c post-remove at removal time, once the original extracted package
     * directory no longer exists.
     *
     * @param root_path Target filesystem root path (e.g. @c "/").
     * @param pkg_name  Name of the package.
     * @return Heap-allocated path, or NULL on allocation failure.
     *         Caller must free().
     */
    APG_API char *scripts_store_path(const char *root_path,
                                     const char *pkg_name);

    /**
     * @brief Persist a package's scripts/ directory beyond the lifetime of its
     *        extracted package directory, so @c pre-remove and @c post-remove
     *        can still be run when the package is later removed.
     *
     * Does nothing (and returns true) if @p pkg_dir has no @c scripts/
     * subdirectory.
     *
     * @param pkg_dir   Path to the extracted package directory, still on disk.
     * @param root_path Target filesystem root path (e.g. @c "/").
     * @param pkg_name  Name of the package being installed.
     * @return true on success or if there was nothing to persist, false if the
     *         scripts/ directory exists but could not be copied.
     */
    APG_API bool scripts_persist(const char *pkg_dir, const char *root_path,
                                 const char *pkg_name);

    /**
     * @brief Remove a package's persisted scripts/ directory.
     *
     * Call after running @c post-remove during package removal. Safe to call
     * even if nothing was ever persisted.
     *
     * @param root_path Target filesystem root path (e.g. @c "/").
     * @param pkg_name  Name of the package being removed.
     */
    APG_API void scripts_persist_remove(const char *root_path,
                                        const char *pkg_name);

#ifdef __cplusplus
}
#endif
