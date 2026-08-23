// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sat_solve_priv.h"

struct sat_solve_ctx
{
    const struct sat_model *m;
    int *assign;
    size_t *antecedent;
    size_t budget;
    size_t decisions;
    bool timed_out;
    bool has_conflict;
    size_t conflict_clause;
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
            {
                size_t best = c;
                for (size_t i = 0; i < clause->count; i++)
                {
                    int var = clause->lits[i] > 0 ? clause->lits[i]
                                                  : -clause->lits[i];
                    size_t ante = ctx->antecedent[var];
                    if (ante != SIZE_MAX &&
                        ctx->m->clauses[ante].kind != SAT_CLAUSE_FORCED)
                    {
                        best = ante;
                        break;
                    }
                }
                ctx->has_conflict = true;
                ctx->conflict_clause = best;
                return false;
            }
            if (unassigned_count == 1)
            {
                int var = unassigned_lit > 0 ? unassigned_lit : -unassigned_lit;
                ctx->assign[var] = unassigned_lit > 0 ? 1 : -1;
                ctx->antecedent[var] = c;
                // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
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
    {
        int var = trail[--(*trail_len)];
        ctx->assign[var] = 0;
        ctx->antecedent[var] = SIZE_MAX;
    }
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

static char *
fmt(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);
    // NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized)
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if (needed < 0)
    {
        va_end(args);
        return NULL;
    }

    char *buf = malloc((size_t)needed + 1);
    if (buf)
        (void)vsnprintf(buf, (size_t)needed + 1, format, args);
    va_end(args);
    return buf;
}

static const char *
pkg_name_or_unknown(const struct package_metadata *pkg)
{
    return pkg->name ? pkg->name : "?";
}

static char *
explain_conflict(const struct sat_model *m, size_t clause_idx)
{
    const struct sat_clause *c = &m->clauses[clause_idx];
    if (c->count == 0)
        return fmt("model contains an empty clause");

    int var0 = c->lits[0] > 0 ? c->lits[0] : -c->lits[0];
    const char *name0 = pkg_name_or_unknown(m->vars[var0 - 1]);

    switch (c->kind)
    {
    case SAT_CLAUSE_DEPENDENCY:
        return fmt("package \"%s\" requires \"%s\", but no candidate could be "
                   "selected",
                   name0, c->dep_name ? c->dep_name : "?");
    case SAT_CLAUSE_CONFLICT:
    {
        int var1 = c->lits[1] > 0 ? c->lits[1] : -c->lits[1];
        const char *name1 = pkg_name_or_unknown(m->vars[var1 - 1]);
        return fmt(
            "package \"%s\" conflicts with package \"%s\", but both were "
            "required",
            name0, name1);
    }
    case SAT_CLAUSE_FORCED:
    default:
        return fmt("package \"%s\" was required but could not be selected",
                   name0);
    }
}

enum sat_result
sat_solve(const struct sat_model *m, size_t decision_budget,
          int **out_assignment, char **out_conflict)
{
    *out_assignment = NULL;
    if (out_conflict)
        *out_conflict = NULL;

    if (m->var_count == 0)
        return SAT_RESULT_SATISFIABLE;

    int *assign = calloc(m->var_count + 1, sizeof(*assign));
    size_t *antecedent = malloc((m->var_count + 1) * sizeof(*antecedent));
    int *trail = malloc(m->var_count * sizeof(*trail));
    if (!assign || !antecedent || !trail)
    {
        free(assign);
        free(antecedent);
        free(trail);
        return SAT_RESULT_ERROR;
    }
    for (size_t v = 0; v <= m->var_count; v++)
        antecedent[v] = SIZE_MAX;

    struct sat_solve_ctx ctx = {m, assign, antecedent, decision_budget,
                                0, false,  false,      0};
    size_t trail_len = 0;
    bool sat = dpll(&ctx, trail, &trail_len);
    free(trail);

    if (ctx.timed_out)
    {
        free(assign);
        free(antecedent);
        return SAT_RESULT_BUDGET_EXCEEDED;
    }

    if (!sat)
    {
        free(assign);
        if (out_conflict && ctx.has_conflict)
            *out_conflict = explain_conflict(m, ctx.conflict_clause);
        free(antecedent);
        return SAT_RESULT_UNSATISFIABLE;
    }
    free(antecedent);

    *out_assignment = assign;
    return SAT_RESULT_SATISFIABLE;
}
