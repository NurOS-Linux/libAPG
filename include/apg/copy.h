// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file copy.h
 * @brief Recursive directory copy utility.
 */

#include "export.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Recursively copy a directory tree.
     *
     * Creates @p dst if it does not exist. Existing files in @p dst are
     * overwritten. Permissions and timestamps are preserved.
     *
     * @param src Source directory path.
     * @param dst Destination directory path.
     * @return true on success, false if any file or directory could not be
     * copied.
     */
    APG_API bool copy_dir(const char *src, const char *dst);

#ifdef __cplusplus
}
#endif
