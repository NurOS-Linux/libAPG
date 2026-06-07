// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file install.h
 * @brief Low-level file installation and rollback.
 */

#include <stdbool.h>

/**
 * @brief Install the @c data/ subtree of a package into a filesystem root.
 *
 * Recursively copies @c pkg_dir/data/ into @p root_path, preserving
 * permissions and ownership.
 *
 * @param pkg_dir   Path to the extracted package directory.
 * @param root_path Destination filesystem root (e.g. @c "/").
 * @return true on success, false if any file could not be copied.
 */
bool install_data_dir(const char *pkg_dir, const char *root_path);

/**
 * @brief Install the @c home/ subtree of a package into the current user's
 *        home directory.
 *
 * @param pkg_dir Path to the extracted package directory.
 * @return true on success, false if any file could not be copied.
 */
bool install_home_dir(const char *pkg_dir);

/**
 * @brief Undo a partial installation by removing files that were already
 *        copied.
 *
 * Removes files from @p root_path that correspond to entries in
 * @c pkg_dir/data/. Errors during removal are ignored.
 *
 * @param pkg_dir   Path to the extracted package directory.
 * @param root_path Filesystem root that was passed to install_data_dir().
 */
void rollback_install(const char *pkg_dir, const char *root_path);
