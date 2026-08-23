// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "candidates_priv.h"

enum sat_clause_kind
{
    SAT_CLAUSE_DEPENDENCY,
    SAT_CLAUSE_CONFLICT,
    SAT_CLAUSE_FORCED,
};

struct sat_clause
{
    int *lits;
    size_t count;
    enum sat_clause_kind kind;
    const char *dep_name;
};

struct sat_model
{
    const struct package_metadata **vars;
    size_t var_count;
    size_t var_cap;

    struct sat_clause *clauses;
    size_t clause_count;
    size_t clause_cap;
};

bool sat_model_init(struct sat_model *m);

void sat_model_free(struct sat_model *m);

int sat_model_var(struct sat_model *m, const struct package_metadata *pkg);

bool sat_model_force(struct sat_model *m, const struct package_metadata *pkg);

bool sat_model_build(struct sat_model *m, const struct candidate_set *cs,
                     const struct package_metadata **pkgs, size_t pkg_count);
