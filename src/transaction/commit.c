// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "trans_priv.h"
#include "../db/db_priv.h"
#include "../../include/apg/package.h"
#include "../../include/apg/db.h"
#include "../../include/apg/journal.h"
#include "../../include/apg/keyring.h"
#include "../../include/util.h"

#define DEFAULT_KEYRING_DIR "/etc/apg/trusted.d"

static void
rollback_committed(struct apg_trans *trans, size_t *committed_idx,
                   size_t committed_count, const char *root_path)
{
    for (size_t j = committed_count; j-- > 0;)
    {
        size_t idx = committed_idx[j];
        struct trans_step *s = &trans->plan[idx];
        struct package *pkg = trans->plan_pkgs[idx];

        if (pkg && root_path)
        {
            const struct str_list *files = &pkg->package_files;
            for (int k = 0; k < files->count; k++)
            {
                if (!files->items[k])
                    continue;
                char *path = concat_dirs(root_path, files->items[k]);
                if (path)
                {
                    unlink(path);
                    free(path);
                }
            }
        }

        db_remove(trans->db, s->pkg_name);
        journal_write(trans->db->env, JOURNAL_ROLLBACK, s->pkg_name,
                      s->pkg_version, JOURNAL_STATUS_OK, getuid(), false);
    }
}

trans_error_t
trans_commit(struct apg_trans *trans, const char *root_path)
{
    if (!trans || !root_path)
        return TRANS_ERR_NOMEM;
    if (!trans->prepared)
        return TRANS_ERR_NOT_PREPARED;
    if (trans->committed)
        return TRANS_ERR_ALREADY_COMMITTED;

    struct keyring *kr = NULL;
    if (trans->require_signature)
    {
        const char *kdir =
            trans->keyring_dir ? trans->keyring_dir : DEFAULT_KEYRING_DIR;
        kr = keyring_load(kdir);
        if (!kr)
            return TRANS_ERR_UNSIGNED;
    }

    size_t *committed_idx =
        trans->plan_count > 0
            ? malloc(trans->plan_count * sizeof(*committed_idx))
            : NULL;
    if (!committed_idx && trans->plan_count > 0)
    {
        keyring_free(kr);
        return TRANS_ERR_NOMEM;
    }

    size_t committed_count = 0;
    uid_t uid = getuid();

    trans->db->suppress_journal = true;

    for (size_t i = 0; i < trans->plan_count; i++)
    {
        struct trans_step *step = &trans->plan[i];

        if (step->op == TRANS_OP_INSTALL)
        {
            struct package *pkg = trans->plan_pkgs[i];

            if (kr)
            {
                char sig_path[PATH_MAX];
                if (!pkg->pkg_path ||
                    snprintf(sig_path, sizeof(sig_path), "%s.sig",
                             pkg->pkg_path) >= (int)sizeof(sig_path) ||
                    !keyring_verify(kr, pkg->pkg_path, sig_path))
                {
                    journal_write(trans->db->env, JOURNAL_INSTALL,
                                  step->pkg_name, step->pkg_version,
                                  JOURNAL_STATUS_FAILED, uid, step->explicit);
                    rollback_committed(trans, committed_idx, committed_count,
                                       root_path);
                    free(committed_idx);
                    keyring_free(kr);
                    trans->db->suppress_journal = false;
                    return TRANS_ERR_UNSIGNED;
                }
            }

            if (!install_package_in_root(pkg, root_path))
            {
                journal_write(trans->db->env, JOURNAL_INSTALL, step->pkg_name,
                              step->pkg_version, JOURNAL_STATUS_FAILED, uid,
                              step->explicit);
                rollback_committed(trans, committed_idx, committed_count,
                                   root_path);
                free(committed_idx);
                keyring_free(kr);
                trans->db->suppress_journal = false;
                return TRANS_ERR_INSTALL_FAILED;
            }

            pkg->installed_by_hand = step->explicit;
            db_add(trans->db, pkg);
            journal_write(trans->db->env, JOURNAL_INSTALL, step->pkg_name,
                          step->pkg_version, JOURNAL_STATUS_OK, uid,
                          step->explicit);
            committed_idx[committed_count++] = i;
        }
        else if (step->op == TRANS_OP_UPGRADE)
        {
            struct package *pkg = trans->plan_pkgs[i];

            if (kr)
            {
                char sig_path[PATH_MAX];
                if (!pkg->pkg_path ||
                    snprintf(sig_path, sizeof(sig_path), "%s.sig",
                             pkg->pkg_path) >= (int)sizeof(sig_path) ||
                    !keyring_verify(kr, pkg->pkg_path, sig_path))
                {
                    journal_write(trans->db->env, JOURNAL_INSTALL,
                                  step->pkg_name, step->pkg_version,
                                  JOURNAL_STATUS_FAILED, uid, step->explicit);
                    rollback_committed(trans, committed_idx, committed_count,
                                       root_path);
                    free(committed_idx);
                    keyring_free(kr);
                    trans->db->suppress_journal = false;
                    return TRANS_ERR_UNSIGNED;
                }
            }

            if (!install_package_in_root(pkg, root_path))
            {
                journal_write(trans->db->env, JOURNAL_INSTALL, step->pkg_name,
                              step->pkg_version, JOURNAL_STATUS_FAILED, uid,
                              step->explicit);
                rollback_committed(trans, committed_idx, committed_count,
                                   root_path);
                free(committed_idx);
                keyring_free(kr);
                trans->db->suppress_journal = false;
                return TRANS_ERR_INSTALL_FAILED;
            }

            pkg->installed_by_hand = step->explicit;
            db_add(trans->db, pkg);
            journal_write(trans->db->env, JOURNAL_INSTALL, step->pkg_name,
                          step->pkg_version, JOURNAL_STATUS_OK, uid,
                          step->explicit);
            committed_idx[committed_count++] = i;
        }
        else
        {
            bool ok = db_remove(trans->db, step->pkg_name);
            journal_write(trans->db->env, JOURNAL_REMOVE, step->pkg_name,
                          step->pkg_version,
                          ok ? JOURNAL_STATUS_OK : JOURNAL_STATUS_FAILED, uid,
                          step->explicit);
        }
    }

    free(committed_idx);
    keyring_free(kr);
    trans->db->suppress_journal = false;
    trans->committed = true;
    return TRANS_OK;
}
