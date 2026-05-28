// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <apg/package.h>
#include <apg/graph.h>

#define NODEPS      NULL, 0
#define NOCONFLICTS NULL, 0
#define NOPROVIDES  NULL, 0
#define NOREPLACES  NULL, 0

static inline struct package_metadata *
make_pkg(const char *name,
         const char **deps,      int dep_count,
         const char **conflicts, int conflict_count,
         const char **provides,  int provides_count,
         const char **replaces,  int replaces_count)
{
    struct package_metadata *pkg = package_metadata_new();
    assert(pkg);
    pkg->name = strdup(name);

    pkg->dependencies.count = dep_count;
    if (dep_count > 0) {
        pkg->dependencies.items = malloc(dep_count * sizeof(char *));
        for (int i = 0; i < dep_count; i++)
            pkg->dependencies.items[i] = strdup(deps[i]);
    }

    pkg->conflicts.count = conflict_count;
    if (conflict_count > 0) {
        pkg->conflicts.items = malloc(conflict_count * sizeof(char *));
        for (int i = 0; i < conflict_count; i++)
            pkg->conflicts.items[i] = strdup(conflicts[i]);
    }

    pkg->provides.count = provides_count;
    if (provides_count > 0) {
        pkg->provides.items = malloc(provides_count * sizeof(char *));
        for (int i = 0; i < provides_count; i++)
            pkg->provides.items[i] = strdup(provides[i]);
    }

    pkg->replaces.count = replaces_count;
    if (replaces_count > 0) {
        pkg->replaces.items = malloc(replaces_count * sizeof(char *));
        for (int i = 0; i < replaces_count; i++)
            pkg->replaces.items[i] = strdup(replaces[i]);
    }

    return pkg;
}
