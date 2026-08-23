// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>

#include "../../include/apg/graph.h"
#include "candidates_priv.h"
#include "sat_model_priv.h"
#include "sat_solve_priv.h"

static sat_solve_result_t
map_result(enum sat_result r)
{
    switch (r)
    {
    case SAT_RESULT_SATISFIABLE:
        return SAT_SOLVE_SATISFIABLE;
    case SAT_RESULT_UNSATISFIABLE:
        return SAT_SOLVE_UNSATISFIABLE;
    case SAT_RESULT_BUDGET_EXCEEDED:
        return SAT_SOLVE_BUDGET_EXCEEDED;
    case SAT_RESULT_ERROR:
    default:
        return SAT_SOLVE_ERROR;
    }
}

sat_solve_result_t
dep_graph_resolve_sat(const struct package_metadata **candidates,
                      size_t candidate_count,
                      const struct package_metadata **roots, size_t root_count,
                      size_t decision_budget, int thread_count,
                      struct package_metadata ***out_selected,
                      size_t *out_selected_count, char **out_conflict)
{
    *out_selected = NULL;
    *out_selected_count = 0;
    if (out_conflict)
        *out_conflict = NULL;

    struct candidate_set cs;
    if (!candidate_set_init(&cs))
        return SAT_SOLVE_ERROR;

    bool ok = true;
    for (size_t i = 0; i < candidate_count && ok; i++)
        ok = candidate_set_add_package(&cs, candidates[i]);
    if (!ok)
    {
        candidate_set_free(&cs);
        return SAT_SOLVE_ERROR;
    }

    struct sat_model m;
    if (!sat_model_init(&m))
    {
        candidate_set_free(&cs);
        return SAT_SOLVE_ERROR;
    }

    if (!sat_model_build(&m, &cs, candidates, candidate_count))
    {
        sat_model_free(&m);
        candidate_set_free(&cs);
        return SAT_SOLVE_ERROR;
    }

    for (size_t i = 0; i < root_count; i++)
        if (!sat_model_force(&m, roots[i]))
        {
            sat_model_free(&m);
            candidate_set_free(&cs);
            return SAT_SOLVE_ERROR;
        }

    int *assignment = NULL;
    char *conflict = NULL;
    enum sat_result r = sat_solve_parallel(&m, decision_budget, thread_count,
                                           &assignment, &conflict);
    sat_solve_result_t result = map_result(r);

    if (result == SAT_SOLVE_SATISFIABLE)
    {
        size_t count = 0;
        for (size_t i = 0; i < m.var_count; i++)
            if (assignment[i + 1] == 1)
                count++;

        struct package_metadata **selected =
            count ? malloc(count * sizeof(*selected)) : NULL;
        if (count && !selected)
        {
            free(assignment);
            free(conflict);
            sat_model_free(&m);
            candidate_set_free(&cs);
            return SAT_SOLVE_ERROR;
        }

        size_t out_i = 0;
        for (size_t i = 0; i < m.var_count; i++)
            if (assignment[i + 1] == 1)
                selected[out_i++] = (struct package_metadata *)m.vars[i];

        *out_selected = selected;
        *out_selected_count = count;
    }
    else if (out_conflict)
    {
        *out_conflict = conflict;
        conflict = NULL;
    }

    free(assignment);
    free(conflict);
    sat_model_free(&m);
    candidate_set_free(&cs);
    return result;
}
