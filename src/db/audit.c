// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

static struct journal_entry **
keep_matching(struct journal_entry **entries, int *count,
              bool (*matches)(const struct journal_entry *, const void *),
              const void *criterion)
{
    int kept = 0;
    for (int i = 0; i < *count; i++)
    {
        if (matches(entries[i], criterion))
            entries[kept++] = entries[i];
        else
            journal_entry_free(entries[i]);
    }

    *count = kept;
    if (kept == 0)
    {
        free(entries);
        return NULL;
    }
    return entries;
}

static bool
is_since(const struct journal_entry *entry, const void *criterion)
{
    return journal_entry_timestamp(entry) >= *(const time_t *)criterion;
}

static bool
is_for_package(const struct journal_entry *entry, const void *criterion)
{
    const char *name = journal_entry_pkg_name(entry);
    return name && strcmp(name, criterion) == 0;
}

struct journal_entry **
audit_read_since(struct db_handle *db, time_t since, int *count)
{
    struct journal_entry **entries = audit_read_all(db, count);
    if (!entries)
        return NULL;
    return keep_matching(entries, count, is_since, &since);
}

struct journal_entry **
audit_read_for_package(struct db_handle *db, const char *pkg_name, int *count)
{
    if (!pkg_name)
    {
        if (count)
            *count = 0;
        return NULL;
    }
    struct journal_entry **entries = audit_read_all(db, count);
    if (!entries)
        return NULL;
    return keep_matching(entries, count, is_for_package, pkg_name);
}
