// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file scripts.h
 * @brief Pre/post-install script execution.
 */

#include <stdbool.h>

/**
 * @brief Run a lifecycle script from a package's @c scripts/ directory.
 *
 * Executes @c pkg_dir/scripts/name if the file exists and is executable.
 *
 * If @p root_path is provided and is not @c "/" (or @c NULL), the child process
 * calls @c chroot(root_path) and @c chdir("/") before applying sandbox
 * primitives and executing the script. If @c chroot fails (e.g. missing
 * privileges), the call fails closed and returns false without executing the
 * script.
 *
 * On Linux, the script runs in a child process with isolated network, mount,
 * UTS, and IPC namespaces (@c unshare()). On FreeBSD, the script runs under
 * Capsicum capability mode (@c cap_enter()).
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
bool run_script(const char *pkg_dir, const char *name, const char *root_path);
