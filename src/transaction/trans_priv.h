// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "../../include/apg/transaction.h"

struct apg_trans
{
    struct db_handle *db;

    struct package **install_pkgs;
    size_t install_count;
    size_t install_cap;

    struct package **upgrade_pkgs;
    size_t upgrade_count;
    size_t upgrade_cap;

    char **remove_names;
    size_t remove_count;
    size_t remove_cap;

    struct trans_step *plan;
    size_t plan_count;
    size_t plan_cap;

    // Parallel to plan[]: non-NULL only for TRANS_OP_INSTALL steps.
    // Pointers are borrowed from install_pkgs — not owned by the transaction.
    struct package **plan_pkgs;

    struct trans_conflict *conflicts;
    size_t conflict_count;
    size_t conflict_cap;

    struct trans_blocked_remove *blocked_removes;
    size_t blocked_remove_count;
    size_t blocked_remove_cap;

    bool prepared;
    bool committed;

    bool require_signature;
    char *keyring_dir;
};
