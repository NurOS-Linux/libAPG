// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>

#include "trans_priv.h"
#include "../../include/apg/package.h"
#include "../../include/apg/db.h"

trans_error_t
trans_commit(struct apg_trans *trans, const char *root_path)
{
    if (!trans || !root_path) return TRANS_ERR_NOMEM;
    if (!trans->prepared) return TRANS_ERR_NOT_PREPARED;
    if (trans->committed) return TRANS_ERR_ALREADY_COMMITTED;

    size_t *committed_idx = trans->plan_count > 0
        ? malloc(trans->plan_count * sizeof(*committed_idx))
        : NULL;
    if (!committed_idx && trans->plan_count > 0) return TRANS_ERR_NOMEM;

    size_t committed_count = 0;

    for (size_t i = 0; i < trans->plan_count; i++) {
        struct trans_step *step = &trans->plan[i];

        if (step->op == TRANS_OP_INSTALL) {
            struct package *pkg = trans->plan_pkgs[i];

            if (!install_package_in_root(pkg, root_path)) {
                for (size_t j = committed_count; j-- > 0;)
                    db_remove(trans->db, trans->plan[committed_idx[j]].pkg_name);
                free(committed_idx);
                return TRANS_ERR_INSTALL_FAILED;
            }

            db_add(trans->db, pkg);
            committed_idx[committed_count++] = i;
        } else {
            db_remove(trans->db, step->pkg_name);
        }
    }

    free(committed_idx);
    trans->committed = true;
    return TRANS_OK;
}
