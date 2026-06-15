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
#include "journal.h"

/**
 * @brief Read all audit log entries in chronological order.
 *
 * @param db    Open database handle.
 * @param count Output parameter set to the number of entries.
 * @return Heap-allocated array of heap-allocated entries, or NULL on failure.
 *         Free with journal_free_all().
 */
struct journal_entry **audit_read_all(struct db_handle *db, int *count);
