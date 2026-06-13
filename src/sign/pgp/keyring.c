// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <gpgme.h>

#include "../../../include/apg/keyring.h"

struct keyring
{
    gpgme_ctx_t ctx;
};

struct keyring *
keyring_load(const char *keyring_dir)
{
    struct keyring *kr = malloc(sizeof(*kr));
    if (!kr)
        return NULL;

    if (gpgme_new(&kr->ctx) != GPG_ERR_NO_ERROR)
    {
        free(kr);
        return NULL;
    }

    if (keyring_dir)
        gpgme_ctx_set_engine_info(kr->ctx, GPGME_PROTOCOL_OpenPGP, NULL,
                                  keyring_dir);
    return kr;
}

void
keyring_free(struct keyring *kr)
{
    if (!kr)
        return;
    gpgme_release(kr->ctx);
    free(kr);
}

bool
keyring_verify(const struct keyring *kr, const char *pkg_path,
               const char *sig_path)
{
    if (!kr || !pkg_path || !sig_path)
        return false;

    gpgme_data_t sig, data;
    if (gpgme_data_new_from_file(&sig, sig_path, 1) != GPG_ERR_NO_ERROR)
        return false;
    if (gpgme_data_new_from_file(&data, pkg_path, 1) != GPG_ERR_NO_ERROR)
    {
        gpgme_data_release(sig);
        return false;
    }

    gpgme_error_t err = gpgme_op_verify(kr->ctx, sig, data, NULL);
    gpgme_data_release(sig);
    gpgme_data_release(data);
    if (err != GPG_ERR_NO_ERROR)
        return false;

    gpgme_verify_result_t result = gpgme_op_verify_result(kr->ctx);
    if (!result || !result->signatures)
        return false;

    gpgme_signature_t s = result->signatures;
    return s->status == GPG_ERR_NO_ERROR &&
           s->validity >= GPGME_VALIDITY_MARGINAL;
}

bool
keyring_add_key(const char *keyring_dir, const char *new_key_path,
                const char *key_sig_path, const struct keyring *trusted)
{

    (void)keyring_dir;
    (void)key_sig_path;
    if (!trusted || !new_key_path)
        return false;

    gpgme_data_t key_data;
    if (gpgme_data_new_from_file(&key_data, new_key_path, 1) !=
        GPG_ERR_NO_ERROR)
        return false;

    gpgme_error_t err = gpgme_op_import(trusted->ctx, key_data);
    gpgme_data_release(key_data);
    if (err != GPG_ERR_NO_ERROR)
        return false;

    gpgme_import_result_t import_result = gpgme_op_import_result(trusted->ctx);
    if (!import_result || import_result->imported == 0)
        return false;

    gpgme_import_status_t status = import_result->imports;
    while (status)
    {
        gpgme_key_t key;
        if (gpgme_get_key(trusted->ctx, status->fpr, &key, 0) ==
            GPG_ERR_NO_ERROR)
        {
            gpgme_validity_t v =
                key->uids ? key->uids->validity : GPGME_VALIDITY_UNKNOWN;
            gpgme_key_unref(key);
            if (v >= GPGME_VALIDITY_MARGINAL)
                return true;
        }
        status = status->next;
    }
    return false;
}
