// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>
#include <apg/version.h>

#define FUZZ_MAX_PKGS 8
#define FUZZ_NAME_POOL_SIZE 6

struct byte_reader
{
    const uint8_t *data;
    size_t size;
    size_t pos;
};

static uint8_t
next_byte(struct byte_reader *r)
{
    if (r->pos >= r->size)
        return 0;
    return r->data[r->pos++];
}

static const char *
pool_name(uint8_t idx)
{
    static const char *names[FUZZ_NAME_POOL_SIZE] = {"a", "b", "c",
                                                     "d", "e", "f"};
    return names[idx % FUZZ_NAME_POOL_SIZE];
}

static struct db_handle *
fuzz_db(void)
{
    static struct db_handle *db = NULL;
    if (db)
        return db;

    char path[] = "/tmp/apg-fuzz-txndb-XXXXXX";
    if (!mkdtemp(path))
        return NULL;
    db = db_open(path);
    return db;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct db_handle *db = fuzz_db();
    if (!db)
        return 0;

    struct byte_reader r = {data, size, 0};
    struct package *pkgs[FUZZ_MAX_PKGS] = {0};
    int pkg_count = 1 + (next_byte(&r) % FUZZ_MAX_PKGS);

    for (int i = 0; i < pkg_count; i++)
    {
        struct package *pkg = package_new();
        if (!pkg)
        {
            pkg_count = i;
            break;
        }

        char namebuf[16];
        snprintf(namebuf, sizeof(namebuf), "%s-%d", pool_name(next_byte(&r)),
                 i);
        pkg->meta->name = strdup(namebuf);
        pkg->meta->version = strdup("1.0.0");
        pkg->meta->type = strdup("app");

        int dep_count = next_byte(&r) % 3;
        if (dep_count > 0)
        {
            pkg->meta->dependencies.items =
                malloc((size_t)dep_count * sizeof(struct dep_constraint));
            pkg->meta->dependencies.count = dep_count;
            for (int j = 0; j < dep_count; j++)
                pkg->meta->dependencies.items[j] = (struct dep_constraint){
                    strdup(pool_name(next_byte(&r))), VER_OP_ANY, NULL};
        }

        int conflict_count = next_byte(&r) % 3;
        if (conflict_count > 0)
        {
            pkg->meta->conflicts.items =
                malloc((size_t)conflict_count * sizeof(char *));
            pkg->meta->conflicts.count = conflict_count;
            for (int j = 0; j < conflict_count; j++)
                pkg->meta->conflicts.items[j] =
                    strdup(pool_name(next_byte(&r)));
        }

        if (next_byte(&r) & 1)
        {
            pkg->installed_by_hand = true;
            db_add(db, pkg);
        }

        pkgs[i] = pkg;
    }

    struct apg_trans *trans = trans_new(db);
    if (trans)
    {
        for (int i = 0; i < pkg_count; i++)
            if (next_byte(&r) & 1)
                trans_add_install(trans, pkgs[i]);

        trans_prepare(trans);
        trans_free(trans);
    }

    for (int i = 0; i < pkg_count; i++)
    {
        db_remove(db, pkgs[i]->meta->name);
        package_free(pkgs[i]);
    }

    return 0;
}
