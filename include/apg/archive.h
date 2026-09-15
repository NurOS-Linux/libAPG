// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file archive.h
 * @brief Package archive extraction.
 */

#include "export.h"
#include "package.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Extract a package archive to its default installation path.
     *
     * Convenience wrapper around unarchive_package_in_root() that uses the
     * live filesystem root @c "/".
     *
     * @param pkg Package whose archive should be extracted.
     * @return true on success, false on any extraction error.
     */
    APG_API bool unarchive_package(const struct package *pkg);

    /**
     * @brief Extract a package archive into an alternative filesystem root.
     *
     * @param pkg  Package whose archive should be extracted.
     * @param root Filesystem root to extract into (e.g. @c "/mnt").
     * @return true on success, false on any extraction error.
     */
    APG_API bool unarchive_package_in_root(const struct package *pkg,
                                           const char *root);

    /**
     * @brief Describe the most recent archive extraction failure on this
     * thread.
     *
     * Set by unarchive_package() / unarchive_package_in_root() whenever
     * they return false, and cleared at the start of each such call. The
     * returned pointer is valid until the next call to either function on
     * the same thread.
     *
     * @return Error message, or NULL if the most recent call succeeded.
     */
    APG_API const char *archive_last_error(void);

#ifdef __cplusplus
}
#endif
