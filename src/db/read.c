// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <lmdb.h>
#include <stdbool.h>

#include "../../include/apg/db.h"
#include "../../include/apg/package.h"

struct package *
get_all_packages(MDB_env *env)
{
    (void)env;
    return NULL;
}

struct package
get_package_by_name(char *name, MDB_env *env)
{
    (void)name;
    (void)env;
    struct package empty = {0};
    return empty;
}
