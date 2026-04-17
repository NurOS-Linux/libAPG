// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include <sodium.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

#include "../../../include/apg/sign.h"

static const char *key_path = "/etc/apg/keys/";

// libsodium uses Ed25519, which is ECC. allow_rsa has no effect here but is kept for interface compatibility.
bool sign_verify(const char *pkg_path, const char *sig_path, bool allow_rsa)
{
    (void)allow_rsa;

    if (sodium_init() < 0)
        return false;

    FILE *sig_f = fopen(sig_path, "rb");
    if (!sig_f)
        return false;

    unsigned char signature[crypto_sign_BYTES];
    if (fread(signature, 1, crypto_sign_BYTES, sig_f) != crypto_sign_BYTES) {
        fclose(sig_f);
        return false;
    }
    fclose(sig_f);

    char key_file[PATH_MAX];
    snprintf(key_file, sizeof(key_file), "%spublic.key", key_path);

    FILE *key_f = fopen(key_file, "rb");
    if (!key_f)
        return false;

    unsigned char public_key[crypto_sign_PUBLICKEYBYTES];
    if (fread(public_key, 1, crypto_sign_PUBLICKEYBYTES, key_f) != crypto_sign_PUBLICKEYBYTES) {
        fclose(key_f);
        return false;
    }
    fclose(key_f);

    crypto_sign_state state;
    crypto_sign_init(&state);

    FILE *pkg_f = fopen(pkg_path, "rb");
    if (!pkg_f)
        return false;

    unsigned char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), pkg_f)) > 0)
        crypto_sign_update(&state, buffer, bytes_read);
    fclose(pkg_f);

    return crypto_sign_final_verify(&state, signature, public_key) == 0;
}

bool sign_file(const char *pkg_path, const char *sig_path)
{
    if (sodium_init() < 0)
        return false;

    char key_file[PATH_MAX];
    snprintf(key_file, sizeof(key_file), "%ssecret.key", key_path);

    FILE *key_f = fopen(key_file, "rb");
    if (!key_f)
        return false;

    unsigned char secret_key[crypto_sign_SECRETKEYBYTES];
    if (fread(secret_key, 1, crypto_sign_SECRETKEYBYTES, key_f) != crypto_sign_SECRETKEYBYTES) {
        fclose(key_f);
        return false;
    }
    fclose(key_f);

    crypto_sign_state state;
    crypto_sign_init(&state);

    FILE *pkg_f = fopen(pkg_path, "rb");
    if (!pkg_f)
        return false;

    unsigned char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), pkg_f)) > 0)
        crypto_sign_update(&state, buffer, bytes_read);
    fclose(pkg_f);

    unsigned char signature[crypto_sign_BYTES];
    if (crypto_sign_final_create(&state, signature, NULL, secret_key) != 0)
        return false;

    FILE *sig_f = fopen(sig_path, "wb");
    if (!sig_f)
        return false;

    fwrite(signature, 1, crypto_sign_BYTES, sig_f);
    fclose(sig_f);
    return true;
}
