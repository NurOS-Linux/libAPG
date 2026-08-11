// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "sat_model_priv.h"
#include "../../include/apg/version.h"

#define SAT_VAR_INITIAL_CAP 16
#define SAT_CLAUSE_INITIAL_CAP 16

bool
sat_model_init(struct sat_model *m)
{
    m->vars = NULL;
    m->var_count = 0;
    m->var_cap = 0;
    m->clauses = NULL;
    m->clause_count = 0;
    m->clause_cap = 0;
    return true;
}

void
sat_model_free(struct sat_model *m)
{
    for (size_t i = 0; i < m->clause_count; i++)
        free(m->clauses[i].lits);
    free(m->clauses);
    m->clauses = NULL;
    m->clause_count = 0;
    m->clause_cap = 0;

    free(m->vars);
    m->vars = NULL;
    m->var_count = 0;
    m->var_cap = 0;
}

int
sat_model_var(struct sat_model *m, const struct package_metadata *pkg)
{
    for (size_t i = 0; i < m->var_count; i++)
        if (m->vars[i] == pkg)
            return (int)(i + 1);

    if (m->var_count == m->var_cap)
    {
        size_t new_cap = m->var_cap ? m->var_cap * 2 : SAT_VAR_INITIAL_CAP;
        const struct package_metadata **tmp =
            realloc(m->vars, new_cap * sizeof(*tmp));
        if (!tmp)
            return 0;
        m->vars = tmp;
        m->var_cap = new_cap;
    }

    m->vars[m->var_count++] = pkg;
    return (int)m->var_count;
}

static bool
add_clause(struct sat_model *m, const int *lits, size_t count)
{
    if (m->clause_count == m->clause_cap)
    {
        size_t new_cap =
            m->clause_cap ? m->clause_cap * 2 : SAT_CLAUSE_INITIAL_CAP;
        struct sat_clause *tmp = realloc(m->clauses, new_cap * sizeof(*tmp));
        if (!tmp)
            return false;
        m->clauses = tmp;
        m->clause_cap = new_cap;
    }

    int *copy = NULL;
    if (count > 0)
    {
        copy = malloc(count * sizeof(*copy));
        if (!copy)
            return false;
        memcpy(copy, lits, count * sizeof(*copy));
    }

    m->clauses[m->clause_count].lits = copy;
    m->clauses[m->clause_count].count = count;
    m->clause_count++;
    return true;
}

static bool
add_dependency_clauses(struct sat_model *m, const struct candidate_set *cs,
                       const struct package_metadata *pkg)
{
    int pkg_var = sat_model_var(m, pkg);
    if (!pkg_var)
        return false;

    for (int i = 0; i < pkg->dependencies.count; i++)
    {
        const struct dep_constraint *dep = &pkg->dependencies.items[i];
        const struct candidate_group *g = candidate_set_lookup(cs, dep->name);

        int *lits = malloc(((g ? g->count : 0) + 1) * sizeof(*lits));
        if (!lits)
            return false;

        lits[0] = -pkg_var;
        size_t lit_count = 1;
        bool ok = true;

        for (size_t j = 0; g && j < g->count; j++)
        {
            const struct package_metadata *cand = g->items[j];
            if (dep->op != VER_OP_ANY &&
                !ver_satisfies(cand->version, dep->op, dep->version))
                continue;

            int v = sat_model_var(m, cand);
            if (!v)
            {
                ok = false;
                break;
            }
            lits[lit_count++] = v;
        }

        if (ok)
            ok = add_clause(m, lits, lit_count);
        free(lits);
        if (!ok)
            return false;
    }
    return true;
}

static bool
add_conflict_clauses(struct sat_model *m, const struct candidate_set *cs,
                     const struct package_metadata *pkg)
{
    int pkg_var = sat_model_var(m, pkg);
    if (!pkg_var)
        return false;

    for (int i = 0; i < pkg->conflicts.count; i++)
    {
        const char *name = pkg->conflicts.items[i];
        if (!name)
            continue;

        const struct candidate_group *g = candidate_set_lookup(cs, name);
        if (!g)
            continue;

        for (size_t j = 0; j < g->count; j++)
        {
            if (g->items[j] == pkg)
                continue;

            int other_var = sat_model_var(m, g->items[j]);
            if (!other_var)
                return false;

            int lits[2] = {-pkg_var, -other_var};
            if (!add_clause(m, lits, 2))
                return false;
        }
    }
    return true;
}

bool
sat_model_build(struct sat_model *m, const struct candidate_set *cs,
                const struct package_metadata **pkgs, size_t pkg_count)
{
    for (size_t i = 0; i < pkg_count; i++)
    {
        if (!add_dependency_clauses(m, cs, pkgs[i]))
            return false;
        if (!add_conflict_clauses(m, cs, pkgs[i]))
            return false;
    }
    return true;
}
