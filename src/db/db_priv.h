// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <lmdb.h>
#include <pthread.h>
#include <stdbool.h>
#include "../../include/apg/db.h"
#include "../../include/apg/journal.h"
#include "../../include/apg/package.h"

struct db_verify_issue
{
    char *pkg_name;
    char **missing_files;
    int missing_count;
};

struct db_check_issue
{
    db_check_kind_t kind;
    char *pkg_name;
    char *path;
    char *other;
};

struct str_vec
{
    char **items;
    int count;
    int cap;
};

bool str_vec_push(struct str_vec *v, const char *str);

enum db_journal_mode
{
    DB_JOURNAL_NONE,
    DB_JOURNAL_SUCCESS,
    DB_JOURNAL_ALWAYS,
};

int apg_journal_put(MDB_txn *txn, journal_op_t op, const char *pkg_name,
                    const char *pkg_version, journal_status_t status, uid_t uid,
                    bool explicit_op);
bool apg_db_add(struct db_handle *db, struct package *pkg,
                enum db_journal_mode mode, bool explicit_op);
bool apg_db_remove(struct db_handle *db, const char *pkg_name,
                   enum db_journal_mode mode, bool explicit_op,
                   const char *journal_version);

struct db_handle
{
    MDB_env *env;
    MDB_dbi files_dbi;
    bool files_dbi_open;
    MDB_dbi file_owner_dbi;
    bool file_owner_dbi_open;
    bool readonly;
    pthread_mutex_t write_lock;
    struct db_hooks hooks;
    int lock_fd; /**< fd holding flock on db.lock; -1 when not locked. */
};
