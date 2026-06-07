// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file journal.h
 * @brief Persistent operation log stored in the LMDB database.
 */

#include <lmdb.h>
#include <time.h>
#include <stdbool.h>

/**
 * @brief Type of operation recorded in the journal.
 */
typedef enum {
    JOURNAL_INSTALL, /**< A package was installed. */
    JOURNAL_REMOVE,  /**< A package was removed. */
} journal_op_t;

/**
 * @brief Outcome of a journalled operation.
 */
typedef enum {
    JOURNAL_STATUS_OK,     /**< The operation completed successfully. */
    JOURNAL_STATUS_FAILED, /**< The operation failed. */
} journal_status_t;

/**
 * @brief A single journal entry describing one install or remove event.
 *
 * All heap-allocated fields are owned by this structure.
 * Free with journal_entry_free() or journal_free_all().
 */
struct journal_entry {
    journal_op_t op;        /**< Type of operation. */
    char *pkg_name;         /**< Package name. Heap-allocated; may be NULL. */
    char *pkg_version;      /**< Package version string. Heap-allocated; may be NULL. */
    time_t timestamp;       /**< Unix timestamp of the operation. */
    journal_status_t status;/**< Outcome of the operation. */
};

/**
 * @brief Append an entry to the journal.
 *
 * @param env         Open LMDB environment.
 * @param op          Type of operation.
 * @param pkg_name    Package name to record.
 * @param pkg_version Package version to record.
 * @param status      Outcome of the operation.
 * @return true on success, false if the write failed.
 */
bool journal_write(MDB_env *env, journal_op_t op, const char *pkg_name,
                   const char *pkg_version, journal_status_t status);

/**
 * @brief Read all journal entries in chronological order.
 *
 * @param env   Open LMDB environment.
 * @param count Output parameter set to the number of entries.
 * @return Heap-allocated NULL-terminated array of heap-allocated entries, or
 *         NULL on failure. Free with journal_free_all().
 */
struct journal_entry **journal_read_all(MDB_env *env, int *count);

/**
 * @brief Free a single journal entry and its owned fields.
 *
 * @param e Entry to free. May be NULL.
 */
void journal_entry_free(struct journal_entry *e);

/**
 * @brief Free an array of journal entries returned by journal_read_all().
 *
 * @param entries Array of entries to free.
 * @param count   Number of entries in the array.
 */
void journal_free_all(struct journal_entry **entries, int count);
