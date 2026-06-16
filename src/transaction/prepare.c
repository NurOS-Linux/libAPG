// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "trans_priv.h"
#include "../../include/apg/graph.h"
#include "../../include/apg/db.h"

static bool
in_strarray(const char **arr, size_t count, const char *name)
{
    for (size_t i = 0; i < count; i++)
        if (strcmp(arr[i], name) == 0)
            return true;
    return false;
}

static bool
plan_has(const struct apg_trans *trans, const char *name)
{
    for (size_t i = 0; i < trans->plan_count; i++)
        if (strcmp(trans->plan[i].pkg_name, name) == 0)
            return true;
    return false;
}

static bool
is_explicit(const struct apg_trans *trans, const char *name)
{
    for (size_t i = 0; i < trans->install_count; i++)
    {
        const struct package_metadata *m = trans->install_pkgs[i]->meta;
        if (m && m->name && strcmp(m->name, name) == 0)
            return true;
    }
    return false;
}

static struct package *
find_install_pkg(const struct apg_trans *trans, const char *name)
{
    for (size_t i = 0; i < trans->install_count; i++)
    {
        const struct package_metadata *m = trans->install_pkgs[i]->meta;
        if (m && m->name && strcmp(m->name, name) == 0)
            return trans->install_pkgs[i];
    }
    return NULL;
}

static trans_error_t
plan_push(struct apg_trans *trans, trans_op_t op, const char *name,
          const char *version, bool explicit_req, struct package *pkg)
{
    if (!trans->plan)
    {
        trans->plan = malloc(8 * sizeof(*trans->plan));
        trans->plan_pkgs = malloc(8 * sizeof(*trans->plan_pkgs));
        if (!trans->plan || !trans->plan_pkgs)
            return TRANS_ERR_NOMEM;
        trans->plan_cap = 8;
    }
    else if (trans->plan_count == trans->plan_cap)
    {
        size_t new_cap = trans->plan_cap * 2;
        struct trans_step *tp = realloc(trans->plan, new_cap * sizeof(*tp));
        struct package **pp = realloc(trans->plan_pkgs, new_cap * sizeof(*pp));
        if (!tp || !pp)
            return TRANS_ERR_NOMEM;
        trans->plan = tp;
        trans->plan_pkgs = pp;
        trans->plan_cap = new_cap;
    }

    struct trans_step *step = &trans->plan[trans->plan_count];
    step->op = op;
    step->pkg_name = strdup(name);
    step->pkg_version = version ? strdup(version) : NULL;
    step->explicit = explicit_req;
    if (!step->pkg_name)
        return TRANS_ERR_NOMEM;

    trans->plan_pkgs[trans->plan_count] = pkg;
    trans->plan_count++;
    return TRANS_OK;
}

static trans_error_t
conflict_push(struct apg_trans *trans, const char *pkg_name,
              const char *conflicts_with)
{
    if (!trans->conflicts)
    {
        trans->conflicts = malloc(4 * sizeof(*trans->conflicts));
        if (!trans->conflicts)
            return TRANS_ERR_NOMEM;
        trans->conflict_cap = 4;
    }
    else if (trans->conflict_count == trans->conflict_cap)
    {
        size_t new_cap = trans->conflict_cap * 2;
        struct trans_conflict *tmp =
            realloc(trans->conflicts, new_cap * sizeof(*tmp));
        if (!tmp)
            return TRANS_ERR_NOMEM;
        trans->conflicts = tmp;
        trans->conflict_cap = new_cap;
    }

    struct trans_conflict *c = &trans->conflicts[trans->conflict_count];
    c->pkg_name = strdup(pkg_name);
    c->conflicts_with = strdup(conflicts_with);
    if (!c->pkg_name || !c->conflicts_with)
        return TRANS_ERR_NOMEM;
    trans->conflict_count++;
    return TRANS_OK;
}

trans_error_t
trans_prepare(struct apg_trans *trans)
{
    if (!trans)
        return TRANS_ERR_NOMEM;
    if (trans->prepared)
        return TRANS_OK;

    int installed_count = 0;
    struct package **installed = db_list(trans->db, &installed_count);

    const char **installed_names = NULL;
    if (installed_count > 0)
    {
        installed_names =
            malloc((size_t)installed_count * sizeof(*installed_names));
        if (!installed_names)
            goto oom;
        for (int i = 0; i < installed_count; i++)
            installed_names[i] =
                (installed[i]->meta && installed[i]->meta->name)
                    ? installed[i]->meta->name
                    : "";
    }

    struct dep_graph *g = dep_graph_new();
    if (!g)
        goto oom;

    for (size_t i = 0; i < trans->install_count; i++)
        dep_graph_add(g, trans->install_pkgs[i]->meta);

    for (size_t i = 0; i < trans->upgrade_count; i++)
        dep_graph_add(g, trans->upgrade_pkgs[i]->meta);

    for (int i = 0; i < installed_count; i++)
        if (installed[i]->meta)
            dep_graph_add(g, installed[i]->meta);

    trans_error_t ret = TRANS_OK;

    for (size_t i = 0; i < trans->install_count; i++)
    {
        const char *pkg_name = trans->install_pkgs[i]->meta->name;

        char **order = NULL;
        size_t order_count = 0;
        dep_error_t err = dep_graph_resolve(g, pkg_name, &order, &order_count);

        if (err == DEP_ERR_CYCLE)
        {
            ret = TRANS_ERR_CYCLE;
            goto cleanup;
        }
        if (err == DEP_ERR_MISSING)
        {
            ret = TRANS_ERR_MISSING_DEP;
            goto cleanup;
        }
        if (err != DEP_OK)
        {
            ret = TRANS_ERR_NOMEM;
            goto cleanup;
        }

        for (size_t j = 0; j < order_count; j++)
        {
            const char *name = order[j];

            if (in_strarray(installed_names, (size_t)installed_count, name))
                continue;
            if (plan_has(trans, name))
                continue;

            struct package *step_pkg = find_install_pkg(trans, name);
            if (!step_pkg)
                continue;

            const char *version =
                step_pkg->meta ? step_pkg->meta->version : NULL;
            trans_error_t perr =
                plan_push(trans, TRANS_OP_INSTALL, name, version,
                          is_explicit(trans, name), step_pkg);
            if (perr != TRANS_OK)
            {
                free(order);
                ret = perr;
                goto cleanup;
            }
        }
        free(order);
    }

    for (size_t i = 0; i < trans->install_count; i++)
    {
        const char *pkg_name = trans->install_pkgs[i]->meta->name;

        char **breaks = NULL;
        size_t break_count = 0;
        dep_error_t err = dep_graph_find_breaks(g, pkg_name, installed_names,
                                                (size_t)installed_count,
                                                &breaks, &break_count);
        if (err != DEP_OK)
        {
            ret = TRANS_ERR_NOMEM;
            goto cleanup;
        }

        for (size_t j = 0; j < break_count; j++)
        {
            trans_error_t cerr = conflict_push(trans, pkg_name, breaks[j]);
            if (cerr != TRANS_OK)
            {
                free(breaks);
                ret = cerr;
                goto cleanup;
            }
        }
        free(breaks);
    }

    for (size_t i = 0; i < trans->upgrade_count; i++)
    {
        struct package *pkg = trans->upgrade_pkgs[i];
        const char *name = pkg->meta->name;
        const char *version = pkg->meta->version;
        trans_error_t perr =
            plan_push(trans, TRANS_OP_UPGRADE, name, version, true, pkg);
        if (perr != TRANS_OK)
        {
            ret = perr;
            goto cleanup;
        }
    }

    for (size_t i = 0; i < trans->remove_count; i++)
    {
        trans_error_t perr = plan_push(
            trans, TRANS_OP_REMOVE, trans->remove_names[i], NULL, true, NULL);
        if (perr != TRANS_OK)
        {
            ret = perr;
            goto cleanup;
        }
    }

    if (trans->conflict_count > 0)
        ret = TRANS_ERR_CONFLICT;
    else
        trans->prepared = true;

cleanup:
    dep_graph_free(g);
    free(installed_names);
    for (int i = 0; i < installed_count; i++)
        package_free(installed[i]);
    free(installed);
    return ret;

oom:
    free(installed_names);
    for (int i = 0; i < installed_count; i++)
        package_free(installed[i]);
    free(installed);
    return TRANS_ERR_NOMEM;
}
