// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/package.h>
#include <apg/version.h>
#include "../src/graph/candidates_priv.h"
#include "../src/graph/sat_model_priv.h"
#include "../src/graph/sat_solve_priv.h"

#define FUZZ_MAX_PKGS 12
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

static const char *
pool_op(uint8_t idx)
{
    static const char *ops[] = {"",        ">=1.0.0", "<=1.0.0", "==1.0.0",
                                "!=1.0.0", ">1.0.0",  "<1.0.0"};
    return ops[idx % (sizeof(ops) / sizeof(ops[0]))];
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct byte_reader r = {data, size, 0};

    int pkg_count = 1 + (next_byte(&r) % FUZZ_MAX_PKGS);
    struct package_metadata *pkgs_meta[FUZZ_MAX_PKGS];
    const struct package_metadata *pkgs[FUZZ_MAX_PKGS];

    for (int i = 0; i < pkg_count; i++)
    {
        struct package_metadata *m = package_metadata_new();
        if (!m)
        {
            pkg_count = i;
            break;
        }

        char namebuf[16];
        (void)snprintf(namebuf, sizeof(namebuf), "%s-%d",
                       pool_name(next_byte(&r)), i);
        m->name = strdup(namebuf);
        m->version = strdup("1.0.0");

        int dep_count = next_byte(&r) % 3;
        if (dep_count > 0)
        {
            m->dependencies.items =
                malloc((size_t)dep_count * sizeof(struct dep_constraint));
            m->dependencies.count = dep_count;
            for (int j = 0; j < dep_count; j++)
            {
                char depbuf[32];
                (void)snprintf(depbuf, sizeof(depbuf), "%s%s",
                               pool_name(next_byte(&r)),
                               pool_op(next_byte(&r)));
                m->dependencies.items[j] = dep_constraint_parse(depbuf);
            }
        }

        int conflict_count = next_byte(&r) % 3;
        if (conflict_count > 0)
        {
            m->conflicts.items =
                malloc((size_t)conflict_count * sizeof(char *));
            m->conflicts.count = conflict_count;
            for (int j = 0; j < conflict_count; j++)
                m->conflicts.items[j] = strdup(pool_name(next_byte(&r)));
        }

        int provides_count = next_byte(&r) % 2;
        if (provides_count > 0)
        {
            m->provides.items = malloc((size_t)provides_count * sizeof(char *));
            m->provides.count = provides_count;
            for (int j = 0; j < provides_count; j++)
                m->provides.items[j] = strdup(pool_name(next_byte(&r)));
        }

        pkgs_meta[i] = m;
        pkgs[i] = m;
    }

    struct candidate_set cs;
    if (candidate_set_init(&cs))
    {
        bool cs_ok = true;
        for (int i = 0; i < pkg_count && cs_ok; i++)
            cs_ok = candidate_set_add_package(&cs, pkgs_meta[i]);

        if (cs_ok)
        {
            struct sat_model model;
            if (sat_model_init(&model))
            {
                if (sat_model_build(&model, &cs, pkgs, (size_t)pkg_count))
                {
                    for (int i = 0; i < pkg_count; i++)
                        if (next_byte(&r) & 1)
                            sat_model_force(&model, pkgs_meta[i]);

                    int *assignment = NULL;
                    char *conflict = NULL;
                    enum sat_result res =
                        sat_solve(&model, 500, &assignment, &conflict);
                    if (res == SAT_RESULT_SATISFIABLE)
                        free(assignment);
                    free(conflict);

                    int *assignment2 = NULL;
                    char *conflict2 = NULL;
                    enum sat_result res2 = sat_solve_parallel(
                        &model, 500, 3, &assignment2, &conflict2);
                    if ((res == SAT_RESULT_SATISFIABLE ||
                         res == SAT_RESULT_UNSATISFIABLE) &&
                        (res2 == SAT_RESULT_SATISFIABLE ||
                         res2 == SAT_RESULT_UNSATISFIABLE))
                        assert(res == res2);
                    if (res2 == SAT_RESULT_SATISFIABLE)
                        free(assignment2);
                    free(conflict2);
                }
                sat_model_free(&model);
            }
        }
        candidate_set_free(&cs);
    }

    for (int i = 0; i < pkg_count; i++)
        package_metadata_free(pkgs_meta[i]);

    return 0;
}
