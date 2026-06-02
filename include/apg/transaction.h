// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "package.h"
#include "db.h"

typedef enum {
    TRANS_OP_INSTALL,
    TRANS_OP_REMOVE,
} trans_op_t;

typedef enum {
    TRANS_OK = 0,
    TRANS_ERR_NOMEM,
    TRANS_ERR_CONFLICT,
    TRANS_ERR_MISSING_DEP,
    TRANS_ERR_CYCLE,
    TRANS_ERR_NOT_PREPARED,
    TRANS_ERR_ALREADY_COMMITTED,
    TRANS_ERR_INSTALL_FAILED,
} trans_error_t;

struct trans_step {
    trans_op_t op;
    char *pkg_name;
    char *pkg_version;
    bool explicit;
};

struct trans_conflict {
    char *pkg_name;
    char *conflicts_with;
};

struct apg_trans;

struct apg_trans *trans_new(struct db_handle *db);
void trans_free(struct apg_trans *trans);

// The transaction borrows pkg — the caller retains ownership.
trans_error_t trans_add_install(struct apg_trans *trans, struct package *pkg);
trans_error_t trans_add_remove(struct apg_trans *trans, const char *pkg_name);

// Resolve dependencies, detect conflicts, and build the ordered execution plan.
// Must be called before trans_get_plan or trans_commit.
// Returns TRANS_ERR_CONFLICT if conflicts were found; call trans_get_conflicts to inspect them.
trans_error_t trans_prepare(struct apg_trans *trans);

// Inspect the plan after a successful trans_prepare call.
// The returned array is owned by the transaction and is valid until trans_free.
const struct trans_step *trans_get_plan(const struct apg_trans *trans, size_t *count);

// Inspect conflicts detected during trans_prepare.
// The returned array is owned by the transaction and is valid until trans_free.
const struct trans_conflict *trans_get_conflicts(const struct apg_trans *trans, size_t *count);

// Execute the plan. root_path is the filesystem root for installation.
// On failure, DB records for already-committed installs are rolled back.
trans_error_t trans_commit(struct apg_trans *trans, const char *root_path);
