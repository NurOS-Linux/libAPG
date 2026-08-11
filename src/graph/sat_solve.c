// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>

#include "sat_solve_priv.h"

struct sat_solve_ctx
{
    const struct sat_model *m;
    int *assign;
    size_t budget;
    size_t decisions;
    bool timed_out;
};

static bool
propagate(struct sat_solve_ctx *ctx, int *trail, size_t *trail_len)
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (size_t c = 0; c < ctx->m->clause_count; c++)
        {
            const struct sat_clause *clause = &ctx->m->clauses[c];
            bool satisfied = false;
            int unassigned_lit = 0;
            size_t unassigned_count = 0;

            for (size_t i = 0; i < clause->count; i++)
            {
                int lit = clause->lits[i];
                int val = ctx->assign[lit > 0 ? lit : -lit];

                if (val == 0)
                {
                    unassigned_count++;
                    unassigned_lit = lit;
                }
                else if ((lit > 0 && val == 1) || (lit < 0 && val == -1))
                {
                    satisfied = true;
                    break;
                }
            }

            if (satisfied)
                continue;
            if (unassigned_count == 0)
                return false;
            if (unassigned_count == 1)
            {
                int var = unassigned_lit > 0 ? unassigned_lit : -unassigned_lit;
                ctx->assign[var] = unassigned_lit > 0 ? 1 : -1;
                trail[(*trail_len)++] = var;
                changed = true;
            }
        }
    }
    return true;
}

static void
undo_to(struct sat_solve_ctx *ctx, const int *trail, size_t *trail_len,
        size_t mark)
{
    while (*trail_len > mark)
        ctx->assign[trail[--(*trail_len)]] = 0;
}

static bool
dpll(struct sat_solve_ctx *ctx, int *trail, size_t *trail_len)
{
    size_t mark = *trail_len;
    if (!propagate(ctx, trail, trail_len))
    {
        undo_to(ctx, trail, trail_len, mark);
        return false;
    }

    size_t chosen = 0;
    for (size_t v = 1; v <= ctx->m->var_count; v++)
        if (ctx->assign[v] == 0)
        {
            chosen = v;
            break;
        }

    if (!chosen)
        return true;

    if (ctx->decisions >= ctx->budget)
    {
        ctx->timed_out = true;
        undo_to(ctx, trail, trail_len, mark);
        return false;
    }
    ctx->decisions++;

    ctx->assign[chosen] = 1;
    trail[(*trail_len)++] = (int)chosen;
    if (dpll(ctx, trail, trail_len))
        return true;
    undo_to(ctx, trail, trail_len, mark);
    if (ctx->timed_out)
        return false;

    ctx->assign[chosen] = -1;
    trail[(*trail_len)++] = (int)chosen;
    if (dpll(ctx, trail, trail_len))
        return true;
    undo_to(ctx, trail, trail_len, mark);

    return false;
}

enum sat_result
sat_solve(const struct sat_model *m, size_t decision_budget,
          int **out_assignment)
{
    *out_assignment = NULL;

    if (m->var_count == 0)
        return SAT_RESULT_SATISFIABLE;

    int *assign = calloc(m->var_count + 1, sizeof(*assign));
    int *trail = malloc(m->var_count * sizeof(*trail));
    if (!assign || !trail)
    {
        free(assign);
        free(trail);
        return SAT_RESULT_ERROR;
    }

    struct sat_solve_ctx ctx = {m, assign, decision_budget, 0, false};
    size_t trail_len = 0;
    bool sat = dpll(&ctx, trail, &trail_len);
    free(trail);

    if (ctx.timed_out)
    {
        free(assign);
        return SAT_RESULT_BUDGET_EXCEEDED;
    }

    if (!sat)
    {
        free(assign);
        return SAT_RESULT_UNSATISFIABLE;
    }

    *out_assignment = assign;
    return SAT_RESULT_SATISFIABLE;
}
