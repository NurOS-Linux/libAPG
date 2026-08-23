// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sat_model_priv.h"
#include "../../include/apg/version.h"

#define SAT_VAR_INITIAL_CAP 16
#define SAT_CLAUSE_INITIAL_CAP 16
#define VAR_INDEX_INITIAL_BUCKETS 16

struct ptr_index_entry
{
    const struct package_metadata *key;
    int value;
    struct ptr_index_entry *next;
};

static size_t
ptr_hash(const void *p)
{
    uintptr_t v = (uintptr_t)p;
    v ^= v >> 16;
    v *= 0x9e3779b97f4a7c15ULL;
    v ^= v >> 32;
    return (size_t)v;
}

static bool
var_index_rehash(struct sat_model *m, size_t new_bucket_count)
{
    struct ptr_index_entry **new_buckets =
        calloc(new_bucket_count, sizeof(*new_buckets));
    if (!new_buckets)
        return false;

    for (size_t i = 0; i < m->var_bucket_count; i++)
    {
        struct ptr_index_entry *e = m->var_buckets[i];
        while (e)
        {
            struct ptr_index_entry *next = e->next;
            size_t idx = ptr_hash(e->key) % new_bucket_count;
            e->next = new_buckets[idx];
            new_buckets[idx] = e;
            e = next;
        }
    }

    free(m->var_buckets);
    m->var_buckets = new_buckets;
    m->var_bucket_count = new_bucket_count;
    return true;
}

bool
sat_model_init(struct sat_model *m)
{
    m->vars = NULL;
    m->var_count = 0;
    m->var_cap = 0;
    m->var_buckets = NULL;
    m->var_bucket_count = 0;
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

    for (size_t i = 0; i < m->var_bucket_count; i++)
    {
        struct ptr_index_entry *e = m->var_buckets[i];
        while (e)
        {
            struct ptr_index_entry *next = e->next;
            free(e);
            e = next;
        }
    }
    free(m->var_buckets);
    m->var_buckets = NULL;
    m->var_bucket_count = 0;

    free(m->vars);
    m->vars = NULL;
    m->var_count = 0;
    m->var_cap = 0;
}

int
sat_model_var(struct sat_model *m, const struct package_metadata *pkg)
{
    if (m->var_bucket_count > 0)
    {
        size_t idx = ptr_hash(pkg) % m->var_bucket_count;
        for (struct ptr_index_entry *e = m->var_buckets[idx]; e; e = e->next)
            if (e->key == pkg)
                return e->value;
    }

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

    if (m->var_count + 1 > m->var_bucket_count - (m->var_bucket_count / 4))
    {
        size_t new_bucket_count = m->var_bucket_count
                                      ? m->var_bucket_count * 2
                                      : VAR_INDEX_INITIAL_BUCKETS;
        if (!var_index_rehash(m, new_bucket_count))
            return 0;
    }

    struct ptr_index_entry *entry = malloc(sizeof(*entry));
    if (!entry)
        return 0;

    m->vars[m->var_count++] = pkg;
    int var = (int)m->var_count;

    entry->key = pkg;
    entry->value = var;
    // NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
    size_t idx = ptr_hash(pkg) % m->var_bucket_count;
    entry->next = m->var_buckets[idx];
    m->var_buckets[idx] = entry;

    return var;
}

static bool
add_clause(struct sat_model *m, const int *lits, size_t count,
           enum sat_clause_kind kind, const char *dep_name)
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
    m->clauses[m->clause_count].kind = kind;
    m->clauses[m->clause_count].dep_name = dep_name;
    m->clause_count++;
    return true;
}

bool
sat_model_force(struct sat_model *m, const struct package_metadata *pkg)
{
    int var = sat_model_var(m, pkg);
    if (!var)
        return false;

    int lits[1] = {var};
    return add_clause(m, lits, 1, SAT_CLAUSE_FORCED, NULL);
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
            ok = add_clause(m, lits, lit_count, SAT_CLAUSE_DEPENDENCY,
                            dep->name);
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
            if (!add_clause(m, lits, 2, SAT_CLAUSE_CONFLICT, NULL))
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
