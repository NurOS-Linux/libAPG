// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>

struct str_map_entry
{
    char *key;
    size_t value;
    struct str_map_entry *next;
};

struct str_map
{
    struct str_map_entry **buckets;
    size_t bucket_count;
    size_t entry_count;
};

bool str_map_init(struct str_map *m);
void str_map_free(struct str_map *m);
bool str_map_get(const struct str_map *m, const char *key, size_t *out_value);
bool str_map_set(struct str_map *m, const char *key, size_t value);
