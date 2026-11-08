// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file audit.h
 * @brief High-level audit log API for reading the package operation history.
 *
 * This API wraps the lower-level journal functions and accepts an open
 * db_handle instead of a raw MDB_env, so callers do not need to know
 * anything about the underlying storage backend.
 */

#include "db.h"
#include "export.h"
#include "journal.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Read all audit log entries in chronological order.
     *
     * @param db    Open database handle.
     * @param count Output parameter set to the number of entries.
     * @return Heap-allocated array of heap-allocated entries, or NULL on
     * failure. Free with journal_free_all().
     */
    APG_API struct journal_entry **audit_read_all(struct db_handle *db,
                                                  int *count);

    /**
     * @brief Read the audit log entries written at or after a point in time.
     *
     * Entries are returned in chronological order, as by audit_read_all().
     *
     * @param db    Open database handle.
     * @param since Earliest timestamp to include, in seconds since the epoch.
     * @param count Output parameter set to the number of entries.
     * @return Heap-allocated array of heap-allocated entries, or NULL if there
     *         are none or on failure. Free with journal_free_all().
     */
    APG_API struct journal_entry **audit_read_since(struct db_handle *db,
                                                    time_t since, int *count);

    /**
     * @brief Read the audit log entries of one package.
     *
     * Returns the entries whose package name equals @p pkg_name exactly, in
     * chronological order.
     *
     * @param db       Open database handle.
     * @param pkg_name Package name to select.
     * @param count    Output parameter set to the number of entries.
     * @return Heap-allocated array of heap-allocated entries, or NULL if there
     *         are none, if @p pkg_name is NULL, or on failure. Free with
     *         journal_free_all().
     */
    APG_API struct journal_entry **audit_read_for_package(struct db_handle *db,
                                                          const char *pkg_name,
                                                          int *count);

#ifdef __cplusplus
}
#endif
