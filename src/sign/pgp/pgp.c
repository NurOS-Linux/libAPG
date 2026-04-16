// Author: AnmiTaliDev <anmitalidev@nuros.org>
#include <gpgme.h>
#include <stdio.h>
#include <stdbool.h>

#include "../../../include/apg/sign.h"

static bool is_ecc_algo(gpgme_pubkey_algo_t algo)
{
    return algo == GPGME_PK_ECC ||
           algo == GPGME_PK_ECDSA ||
           algo == GPGME_PK_EDDSA;
}

bool sign_verify(const char *pkg_path, const char *sig_path, bool allow_rsa)
{
    gpgme_ctx_t ctx;
    gpgme_data_t sig, data;

    if (gpgme_new(&ctx) != GPG_ERR_NO_ERROR)
        return false;

    if (gpgme_data_new_from_file(&sig, sig_path, 1) != GPG_ERR_NO_ERROR) {
        gpgme_release(ctx);
        return false;
    }

    if (gpgme_data_new_from_file(&data, pkg_path, 1) != GPG_ERR_NO_ERROR) {
        gpgme_data_release(sig);
        gpgme_release(ctx);
        return false;
    }

    gpgme_error_t err = gpgme_op_verify(ctx, sig, data, NULL);
    gpgme_data_release(sig);
    gpgme_data_release(data);

    if (err != GPG_ERR_NO_ERROR) {
        gpgme_release(ctx);
        return false;
    }

    gpgme_verify_result_t result = gpgme_op_verify_result(ctx);
    if (!result || !result->signatures) {
        gpgme_release(ctx);
        return false;
    }

    gpgme_signature_t s = result->signatures;
    bool valid = s->status == GPG_ERR_NO_ERROR;

    if (valid && !allow_rsa)
        valid = is_ecc_algo(s->pubkey_algo);

    gpgme_release(ctx);
    return valid;
}

bool sign_file(const char *pkg_path, const char *sig_path)
{
    gpgme_ctx_t ctx;
    gpgme_data_t in, out;

    if (gpgme_new(&ctx) != GPG_ERR_NO_ERROR)
        return false;

    gpgme_set_armor(ctx, 0);

    if (gpgme_data_new_from_file(&in, pkg_path, 1) != GPG_ERR_NO_ERROR) {
        gpgme_release(ctx);
        return false;
    }

    if (gpgme_data_new(&out) != GPG_ERR_NO_ERROR) {
        gpgme_data_release(in);
        gpgme_release(ctx);
        return false;
    }

    gpgme_error_t err = gpgme_op_sign(ctx, in, out, GPGME_SIG_MODE_DETACH);
    gpgme_data_release(in);

    if (err != GPG_ERR_NO_ERROR) {
        gpgme_data_release(out);
        gpgme_release(ctx);
        return false;
    }

    gpgme_data_seek(out, 0, SEEK_SET);

    FILE *f = fopen(sig_path, "wb");
    if (!f) {
        gpgme_data_release(out);
        gpgme_release(ctx);
        return false;
    }

    char buf[4096];
    ssize_t n;
    while ((n = gpgme_data_read(out, buf, sizeof(buf))) > 0)
        fwrite(buf, 1, (size_t)n, f);

    fclose(f);
    gpgme_data_release(out);
    gpgme_release(ctx);
    return true;
}
