// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "trans_priv.h"

#define TRANS_INITIAL_CAP 8

struct apg_trans *
trans_new(struct db_handle *db)
{
    if (!db)
        return NULL;

    struct apg_trans *trans = calloc(1, sizeof(*trans));
    if (!trans)
        return NULL;

    trans->install_pkgs =
        malloc(TRANS_INITIAL_CAP * sizeof(*trans->install_pkgs));
    if (!trans->install_pkgs)
        goto fail;
    trans->install_cap = TRANS_INITIAL_CAP;

    trans->upgrade_pkgs =
        malloc(TRANS_INITIAL_CAP * sizeof(*trans->upgrade_pkgs));
    if (!trans->upgrade_pkgs)
        goto fail;
    trans->upgrade_cap = TRANS_INITIAL_CAP;

    trans->remove_names =
        malloc(TRANS_INITIAL_CAP * sizeof(*trans->remove_names));
    if (!trans->remove_names)
        goto fail;
    trans->remove_cap = TRANS_INITIAL_CAP;

    trans->db = db;
    return trans;

fail:
    free(trans->install_pkgs);
    free(trans->upgrade_pkgs);
    free(trans->remove_names);
    free(trans);
    return NULL;
}

void
trans_set_policy(struct apg_trans *trans, const install_policy *policy)
{
    if (!trans)
        return;
    free(trans->keyring_dir);
    trans->keyring_dir = NULL;
    if (!policy)
    {
        trans->require_signature = false;
        return;
    }
    trans->require_signature = policy->require_signature;
    if (policy->keyring_dir)
        trans->keyring_dir = strdup(policy->keyring_dir);
}

void
trans_free(struct apg_trans *trans)
{
    if (!trans)
        return;

    free(trans->keyring_dir);
    free(trans->install_pkgs);
    free(trans->upgrade_pkgs);

    for (size_t i = 0; i < trans->remove_count; i++)
        free(trans->remove_names[i]);
    free(trans->remove_names);

    for (size_t i = 0; i < trans->plan_count; i++)
    {
        free(trans->plan[i].pkg_name);
        free(trans->plan[i].pkg_version);
    }
    free(trans->plan);
    free(trans->plan_pkgs);

    for (size_t i = 0; i < trans->conflict_count; i++)
    {
        free(trans->conflicts[i].pkg_name);
        free(trans->conflicts[i].conflicts_with);
    }
    free(trans->conflicts);

    free(trans);
}

trans_error_t
trans_add_install(struct apg_trans *trans, struct package *pkg)
{
    if (!trans || !pkg || !pkg->meta || !pkg->meta->name)
        return TRANS_ERR_NOMEM;

    if (trans->install_count == trans->install_cap)
    {
        size_t new_cap = trans->install_cap * 2;
        struct package **tmp =
            realloc(trans->install_pkgs, new_cap * sizeof(*tmp));
        if (!tmp)
            return TRANS_ERR_NOMEM;
        trans->install_pkgs = tmp;
        trans->install_cap = new_cap;
    }

    trans->install_pkgs[trans->install_count++] = pkg;
    return TRANS_OK;
}

trans_error_t
trans_add_upgrade(struct apg_trans *trans, struct package *pkg)
{
    if (!trans || !pkg || !pkg->meta || !pkg->meta->name)
        return TRANS_ERR_NOMEM;

    if (trans->upgrade_count == trans->upgrade_cap)
    {
        size_t new_cap = trans->upgrade_cap * 2;
        struct package **tmp =
            realloc(trans->upgrade_pkgs, new_cap * sizeof(*tmp));
        if (!tmp)
            return TRANS_ERR_NOMEM;
        trans->upgrade_pkgs = tmp;
        trans->upgrade_cap = new_cap;
    }

    trans->upgrade_pkgs[trans->upgrade_count++] = pkg;
    return TRANS_OK;
}

trans_error_t
trans_add_remove(struct apg_trans *trans, const char *pkg_name)
{
    if (!trans || !pkg_name)
        return TRANS_ERR_NOMEM;

    if (trans->remove_count == trans->remove_cap)
    {
        size_t new_cap = trans->remove_cap * 2;
        char **tmp = realloc(trans->remove_names, new_cap * sizeof(*tmp));
        if (!tmp)
            return TRANS_ERR_NOMEM;
        trans->remove_names = tmp;
        trans->remove_cap = new_cap;
    }

    char *dup = strdup(pkg_name);
    if (!dup)
        return TRANS_ERR_NOMEM;
    trans->remove_names[trans->remove_count++] = dup;
    return TRANS_OK;
}

const struct trans_step *
trans_get_plan(const struct apg_trans *trans, size_t *count)
{
    if (!trans || !count)
        return NULL;
    *count = trans->plan_count;
    return trans->plan;
}

const struct trans_conflict *
trans_get_conflicts(const struct apg_trans *trans, size_t *count)
{
    if (!trans || !count)
        return NULL;
    *count = trans->conflict_count;
    return trans->conflicts;
}
