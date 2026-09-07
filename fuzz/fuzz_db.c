// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/db.h>
#include <apg/package.h>

#define FUZZ_NAME_POOL_SIZE 8

static struct db_handle *
fuzz_db(void)
{
    static struct db_handle *db = NULL;
    if (db)
        return db;

    char path[] = "/tmp/apg-fuzz-db-XXXXXX";
    if (!mkdtemp(path))
        return NULL;
    db = db_open(path);
    return db;
}

static const char *
pool_name(uint8_t idx)
{
    static const char *names[FUZZ_NAME_POOL_SIZE] = {
        "pkg-0", "pkg-1", "pkg-2", "pkg-3", "pkg-4", "pkg-5", "pkg-6", "pkg-7",
    };
    return names[idx % FUZZ_NAME_POOL_SIZE];
}

static char *
bounded_str(const uint8_t *data, size_t size, size_t *pos, size_t max_len)
{
    size_t len = *pos < size ? data[*pos] % max_len : 0;
    if (*pos < size)
        (*pos)++;
    if (*pos + len > size)
        len = size > *pos ? size - *pos : 0;

    char *s = malloc(len + 1);
    if (!s)
        return NULL;
    if (len > 0)
        memcpy(s, data + *pos, len);
    s[len] = '\0';
    *pos += len;
    return s;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct db_handle *db = fuzz_db();
    if (!db)
        return 0;

    if (size < 2)
        return 0;

    size_t pos = 0;
    const char *name = pool_name(data[pos++]);

    struct package *pkg = package_new();
    if (!pkg)
        return 0;

    pkg->meta->name = strdup(name);
    pkg->meta->version = bounded_str(data, size, &pos, 32);
    pkg->meta->description = bounded_str(data, size, &pos, 64);
    pkg->installed_by_hand = pos < size && (data[pos++] & 1);

    int dep_count = pos < size ? data[pos++] % 3 : 0;
    if (dep_count > 0)
    {
        pkg->meta->dependencies.items =
            malloc((size_t)dep_count * sizeof(struct dep_constraint));
        pkg->meta->dependencies.count = dep_count;
        for (int i = 0; i < dep_count; i++)
        {
            char *dep_name = bounded_str(data, size, &pos, 16);
            pkg->meta->dependencies.items[i] =
                (struct dep_constraint){dep_name, VER_OP_ANY, NULL};
        }
    }

    if (db_add(db, pkg))
    {
        struct package *fetched = db_get(db, name);
        if (fetched)
        {
            if (fetched->meta && fetched->meta->name)
            {
                if (strcmp(fetched->meta->name, name) != 0)
                    abort();
            }
            package_free(fetched);
        }
    }

    package_free(pkg);
    return 0;
}
