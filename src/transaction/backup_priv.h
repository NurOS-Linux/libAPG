// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

#include "../../include/apg/db.h"
#include "../../include/apg/package.h"

struct upgrade_backup
{
    struct package *previous;
    char *dir;
    bool had_scripts;
};

bool upgrade_backup_create(struct db_handle *db, const char *pkg_name,
                           const char *root_path, struct upgrade_backup *out);

void upgrade_backup_restore(struct db_handle *db,
                            const struct upgrade_backup *backup,
                            const struct package *new_pkg,
                            const char *root_path);

void upgrade_backup_discard(struct upgrade_backup *backup);
