// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
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
    const atomic_bool *cancel;
    bool aborted;
    bool reverse_order;
    bool prefer_false;
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

static size_t
pick_var(struct sat_solve_ctx *ctx)
{
    if (!ctx->reverse_order)
    {
        for (size_t v = 1; v <= ctx->m->var_count; v++)
            if (ctx->assign[v] == 0)
                return v;
        return 0;
    }

    for (size_t v = ctx->m->var_count; v >= 1; v--)
        if (ctx->assign[v] == 0)
            return v;
    return 0;
}

static bool
dpll(struct sat_solve_ctx *ctx, int *trail, size_t *trail_len)
{
    if (ctx->cancel && atomic_load_explicit(ctx->cancel, memory_order_relaxed))
    {
        ctx->aborted = true;
        return false;
    }

    size_t mark = *trail_len;
    if (!propagate(ctx, trail, trail_len))
    {
        undo_to(ctx, trail, trail_len, mark);
        return false;
    }

    size_t chosen = pick_var(ctx);
    if (!chosen)
        return true;

    if (ctx->decisions >= ctx->budget)
    {
        ctx->timed_out = true;
        undo_to(ctx, trail, trail_len, mark);
        return false;
    }
    ctx->decisions++;

    int first_val = ctx->prefer_false ? -1 : 1;

    ctx->assign[chosen] = first_val;
    // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
    trail[(*trail_len)++] = (int)chosen;
    if (dpll(ctx, trail, trail_len))
        return true;
    undo_to(ctx, trail, trail_len, mark);
    if (ctx->timed_out || ctx->aborted)
        return false;

    ctx->assign[chosen] = -first_val;
    // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
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

static enum sat_result
solve_core(const struct sat_model *m, size_t decision_budget,
           int **out_assignment, char **out_conflict, const atomic_bool *cancel,
           bool reverse_order, bool prefer_false)
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

    struct sat_solve_ctx ctx = {m,      assign, antecedent,    decision_budget,
                                0,      false,  false,         0,
                                cancel, false,  reverse_order, prefer_false};
    size_t trail_len = 0;
    bool sat = dpll(&ctx, trail, &trail_len);
    free(trail);

    if (ctx.aborted)
    {
        free(assign);
        free(antecedent);
        return SAT_RESULT_ERROR;
    }

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

enum sat_result
sat_solve(const struct sat_model *m, size_t decision_budget,
          int **out_assignment, char **out_conflict)
{
    return solve_core(m, decision_budget, out_assignment, out_conflict, NULL,
                      false, false);
}

struct sat_worker_arg
{
    const struct sat_model *m;
    size_t budget;
    bool reverse_order;
    bool prefer_false;
    atomic_bool *cancel;

    pthread_mutex_t *result_lock;
    bool *result_set;
    enum sat_result *final_result;
    int **final_assignment;
    char **final_conflict;
};

static void *
sat_worker_fn(void *raw_arg)
{
    struct sat_worker_arg *arg = raw_arg;
    int *assignment = NULL;
    char *conflict = NULL;
    enum sat_result r =
        solve_core(arg->m, arg->budget, &assignment, &conflict, arg->cancel,
                   arg->reverse_order, arg->prefer_false);

    if (r == SAT_RESULT_SATISFIABLE || r == SAT_RESULT_UNSATISFIABLE)
    {
        pthread_mutex_lock(arg->result_lock);
        if (!*arg->result_set)
        {
            *arg->result_set = true;
            *arg->final_result = r;
            *arg->final_assignment = assignment;
            *arg->final_conflict = conflict;
            assignment = NULL;
            conflict = NULL;
            atomic_store_explicit(arg->cancel, true, memory_order_relaxed);
        }
        pthread_mutex_unlock(arg->result_lock);
    }

    free(assignment);
    free(conflict);
    return NULL;
}

enum sat_result
sat_solve_parallel(const struct sat_model *m, size_t decision_budget,
                   int thread_count, int **out_assignment, char **out_conflict)
{
    if (thread_count <= 1)
        return sat_solve(m, decision_budget, out_assignment, out_conflict);

    *out_assignment = NULL;
    if (out_conflict)
        *out_conflict = NULL;

    pthread_t *threads = malloc((size_t)thread_count * sizeof(*threads));
    struct sat_worker_arg *args = malloc((size_t)thread_count * sizeof(*args));
    bool *started = calloc((size_t)thread_count, sizeof(*started));
    if (!threads || !args || !started)
    {
        free(threads);
        free(args);
        free(started);
        return sat_solve(m, decision_budget, out_assignment, out_conflict);
    }

    atomic_bool cancel = false;
    pthread_mutex_t result_lock;
    pthread_mutex_init(&result_lock, NULL);
    bool result_set = false;
    enum sat_result final_result = SAT_RESULT_ERROR;
    int *final_assignment = NULL;
    char *final_conflict = NULL;

    for (int i = 0; i < thread_count; i++)
    {
        args[i].m = m;
        args[i].budget = decision_budget;
        args[i].reverse_order = (i & 1) != 0;
        args[i].prefer_false = (i & 2) != 0;
        args[i].cancel = &cancel;
        args[i].result_lock = &result_lock;
        args[i].result_set = &result_set;
        args[i].final_result = &final_result;
        args[i].final_assignment = &final_assignment;
        args[i].final_conflict = &final_conflict;

        if (pthread_create(&threads[i], NULL, sat_worker_fn, &args[i]) == 0)
            started[i] = true;
        else
            sat_worker_fn(&args[i]);
    }

    for (int i = 0; i < thread_count; i++)
        if (started[i])
            pthread_join(threads[i], NULL);

    free(started);
    free(threads);
    free(args);
    pthread_mutex_destroy(&result_lock);

    if (!result_set)
        return SAT_RESULT_BUDGET_EXCEEDED;

    *out_assignment = final_assignment;
    if (out_conflict)
        *out_conflict = final_conflict;
    else
        free(final_conflict);
    return final_result;
}
