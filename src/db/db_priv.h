// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <lmdb.h>
#include <pthread.h>
#include <stdbool.h>
#include "../../include/apg/db.h"

struct db_handle {
    MDB_env *env;
    MDB_dbi files_dbi;
    bool files_dbi_open;
    bool readonly;
    pthread_mutex_t write_lock;
    struct db_hooks hooks;
};
