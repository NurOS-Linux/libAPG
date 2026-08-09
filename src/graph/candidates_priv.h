// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stddef.h>
#include "../hashmap_priv.h"
#include "../../include/apg/package.h"

struct candidate_group
{
    char *name;
    const struct package_metadata **items;
    size_t count;
    size_t cap;
};

struct candidate_set
{
    struct candidate_group *groups;
    size_t count;
    size_t cap;
    struct str_map index;
};

bool candidate_set_init(struct candidate_set *cs);

void candidate_set_free(struct candidate_set *cs);

bool candidate_set_add(struct candidate_set *cs, const char *name,
                       const struct package_metadata *pkg);

bool candidate_set_add_package(struct candidate_set *cs,
                               const struct package_metadata *pkg);

const struct candidate_group *
candidate_set_lookup(const struct candidate_set *cs, const char *name);
