// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <lmdb.h>
#include <pthread.h>
#include <stdbool.h>
#include "../../include/apg/db.h"
#include "../../include/apg/package.h"

struct package *db_get(struct db_handle *db, const char *name);

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
    bool suppress_journal; /**< When true, db_add/db_remove skip journal_write.
                                Used by trans_commit which journals manually. */
};
