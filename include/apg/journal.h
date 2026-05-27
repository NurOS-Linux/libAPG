// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <lmdb.h>
#include <time.h>
#include <stdbool.h>

typedef enum {
    JOURNAL_INSTALL,
    JOURNAL_REMOVE,
} journal_op_t;

typedef enum {
    JOURNAL_STATUS_OK,
    JOURNAL_STATUS_FAILED,
} journal_status_t;

struct journal_entry {
    journal_op_t op;
    char *pkg_name;     // owned, may be NULL
    char *pkg_version;  // owned, may be NULL
    time_t timestamp;
    journal_status_t status;
};

bool journal_write(MDB_env *env, journal_op_t op, const char *pkg_name,
                   const char *pkg_version, journal_status_t status);

// Returns chronologically ordered array; caller owns it via journal_free_all.
struct journal_entry **journal_read_all(MDB_env *env, int *count);

void journal_entry_free(struct journal_entry *e);
void journal_free_all(struct journal_entry **entries, int count);
