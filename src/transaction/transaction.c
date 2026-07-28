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
        trans->sign_backend = SIGN_BACKEND_SODIUM;
        return;
    }
    trans->require_signature = policy->require_signature;
    trans->sign_backend = policy->backend;
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

    for (size_t i = 0; i < trans->blocked_remove_count; i++)
    {
        free(trans->blocked_removes[i].pkg_name);
        for (int j = 0; j < trans->blocked_removes[i].dependent_count; j++)
            free(trans->blocked_removes[i].dependents[j]);
        free(trans->blocked_removes[i].dependents);
    }
    free(trans->blocked_removes);

    for (size_t i = 0; i < trans->file_conflict_count; i++)
    {
        free(trans->file_conflicts[i].path);
        free(trans->file_conflicts[i].requested_by);
        free(trans->file_conflicts[i].owned_by);
    }
    free(trans->file_conflicts);

    for (size_t i = 0; i < trans->held_count; i++)
        free(trans->held_pkgs[i].pkg_name);
    free(trans->held_pkgs);

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

size_t
trans_plan_count(const struct apg_trans *trans)
{
    return trans ? trans->plan_count : 0;
}

const struct trans_step *
trans_plan_at(const struct apg_trans *trans, size_t index)
{
    if (!trans || index >= trans->plan_count)
        return NULL;
    return &trans->plan[index];
}

size_t
trans_conflict_count(const struct apg_trans *trans)
{
    return trans ? trans->conflict_count : 0;
}

const struct trans_conflict *
trans_conflict_at(const struct apg_trans *trans, size_t index)
{
    if (!trans || index >= trans->conflict_count)
        return NULL;
    return &trans->conflicts[index];
}

size_t
trans_blocked_remove_count(const struct apg_trans *trans)
{
    return trans ? trans->blocked_remove_count : 0;
}

const struct trans_blocked_remove *
trans_blocked_remove_at(const struct apg_trans *trans, size_t index)
{
    if (!trans || index >= trans->blocked_remove_count)
        return NULL;
    return &trans->blocked_removes[index];
}

size_t
trans_file_conflict_count(const struct apg_trans *trans)
{
    return trans ? trans->file_conflict_count : 0;
}

const struct trans_file_conflict *
trans_file_conflict_at(const struct apg_trans *trans, size_t index)
{
    if (!trans || index >= trans->file_conflict_count)
        return NULL;
    return &trans->file_conflicts[index];
}

size_t
trans_held_pkg_count(const struct apg_trans *trans)
{
    return trans ? trans->held_count : 0;
}

const struct trans_held_pkg *
trans_held_pkg_at(const struct apg_trans *trans, size_t index)
{
    if (!trans || index >= trans->held_count)
        return NULL;
    return &trans->held_pkgs[index];
}

trans_op_t
trans_step_op(const struct trans_step *step)
{
    return step->op;
}

const char *
trans_step_pkg_name(const struct trans_step *step)
{
    return step->pkg_name;
}

const char *
trans_step_pkg_version(const struct trans_step *step)
{
    return step->pkg_version;
}

bool
trans_step_explicit(const struct trans_step *step)
{
    return step->explicit;
}

const char *
trans_conflict_pkg_name(const struct trans_conflict *conflict)
{
    return conflict->pkg_name;
}

const char *
trans_conflict_conflicts_with(const struct trans_conflict *conflict)
{
    return conflict->conflicts_with;
}

const char *
trans_file_conflict_path(const struct trans_file_conflict *conflict)
{
    return conflict->path;
}

const char *
trans_file_conflict_requested_by(const struct trans_file_conflict *conflict)
{
    return conflict->requested_by;
}

const char *
trans_file_conflict_owned_by(const struct trans_file_conflict *conflict)
{
    return conflict->owned_by;
}

const char *
trans_held_pkg_name(const struct trans_held_pkg *held)
{
    return held->pkg_name;
}

trans_op_t
trans_held_pkg_op(const struct trans_held_pkg *held)
{
    return held->op;
}

const char *
trans_blocked_remove_pkg_name(const struct trans_blocked_remove *blocked)
{
    return blocked->pkg_name;
}

int
trans_blocked_remove_dependent_count(const struct trans_blocked_remove *blocked)
{
    return blocked->dependent_count;
}

const char *
trans_blocked_remove_dependent_at(const struct trans_blocked_remove *blocked,
                                  int index)
{
    if (index < 0 || index >= blocked->dependent_count)
        return NULL;
    return blocked->dependents[index];
}
