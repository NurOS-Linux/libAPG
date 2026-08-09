// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "hashmap_priv.h"

#define STR_MAP_INITIAL_BUCKETS 16

static size_t
fnv1a(const char *s)
{
    size_t h = 14695981039346656037ULL;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++)
    {
        h ^= (size_t)*p;
        h *= 1099511628211ULL;
    }
    return h;
}

bool
str_map_init(struct str_map *m)
{
    m->buckets = calloc(STR_MAP_INITIAL_BUCKETS, sizeof(*m->buckets));
    if (!m->buckets)
        return false;
    m->bucket_count = STR_MAP_INITIAL_BUCKETS;
    m->entry_count = 0;
    return true;
}

void
str_map_free(struct str_map *m)
{
    if (!m->buckets)
        return;
    for (size_t i = 0; i < m->bucket_count; i++)
    {
        struct str_map_entry *e = m->buckets[i];
        while (e)
        {
            struct str_map_entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(m->buckets);
    m->buckets = NULL;
    m->bucket_count = 0;
    m->entry_count = 0;
}

bool
str_map_get(const struct str_map *m, const char *key, size_t *out_value)
{
    if (!m->buckets)
        return false;
    size_t idx = fnv1a(key) % m->bucket_count;
    for (struct str_map_entry *e = m->buckets[idx]; e; e = e->next)
    {
        if (strcmp(e->key, key) == 0)
        {
            *out_value = e->value;
            return true;
        }
    }
    return false;
}

static bool
str_map_rehash(struct str_map *m, size_t new_bucket_count)
{
    struct str_map_entry **new_buckets =
        calloc(new_bucket_count, sizeof(*new_buckets));
    if (!new_buckets)
        return false;

    for (size_t i = 0; i < m->bucket_count; i++)
    {
        struct str_map_entry *e = m->buckets[i];
        while (e)
        {
            struct str_map_entry *next = e->next;
            size_t idx = fnv1a(e->key) % new_bucket_count;
            e->next = new_buckets[idx];
            new_buckets[idx] = e;
            e = next;
        }
    }

    free(m->buckets);
    m->buckets = new_buckets;
    m->bucket_count = new_bucket_count;
    return true;
}

bool
str_map_set(struct str_map *m, const char *key, size_t value)
{
    if (!m->buckets)
        return false;

    size_t idx = fnv1a(key) % m->bucket_count;
    for (struct str_map_entry *e = m->buckets[idx]; e; e = e->next)
    {
        if (strcmp(e->key, key) == 0)
        {
            e->value = value;
            return true;
        }
    }

    if (m->entry_count + 1 > m->bucket_count - (m->bucket_count / 4))
    {
        if (!str_map_rehash(m, m->bucket_count * 2))
            return false;
        idx = fnv1a(key) % m->bucket_count;
    }

    struct str_map_entry *e = malloc(sizeof(*e));
    if (!e)
        return false;
    e->key = strdup(key);
    if (!e->key)
    {
        free(e);
        return false;
    }
    e->value = value;
    e->next = m->buckets[idx];
    m->buckets[idx] = e;
    m->entry_count++;
    return true;
}
