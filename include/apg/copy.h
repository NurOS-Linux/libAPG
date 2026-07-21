// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file copy.h
 * @brief Recursive directory copy utility.
 */

#include <stdbool.h>

/**
 * @brief Recursively copy a directory tree.
 *
 * Creates @p dst if it does not exist. Existing files in @p dst are
 * overwritten. Permissions and timestamps are preserved.
 *
 * @param src Source directory path.
 * @param dst Destination directory path.
 * @return true on success, false if any file or directory could not be copied.
 */
bool copy_dir(const char *src, const char *dst);
