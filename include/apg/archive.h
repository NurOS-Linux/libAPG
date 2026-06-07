// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file archive.h
 * @brief Package archive extraction.
 */

#include "package.h"

#include <stdbool.h>

/**
 * @brief Extract a package archive to its default installation path.
 *
 * Convenience wrapper around unarchive_package_in_root() that uses the
 * live filesystem root @c "/".
 *
 * @param pkg Package whose archive should be extracted.
 * @return true on success, false on any extraction error.
 */
bool unarchive_package(const struct package *pkg);

/**
 * @brief Extract a package archive into an alternative filesystem root.
 *
 * @param pkg  Package whose archive should be extracted.
 * @param root Filesystem root to extract into (e.g. @c "/mnt").
 * @return true on success, false on any extraction error.
 */
bool unarchive_package_in_root(const struct package *pkg, const char *root);
