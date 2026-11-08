// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "db_priv.h"
#include "../../include/apg/journal.h"
#include "../../include/apg/audit.h"

struct journal_entry **
audit_read_all(struct db_handle *db, int *count)
{
    if (!db || !count)
    {
        if (count)
            *count = 0;
        return NULL;
    }
    return journal_read_all(db->env, count);
}
