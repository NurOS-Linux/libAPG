// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include <sodium.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

#include "../../../include/apg/sign.h"

static const char *key_path = APG_KEYS_DIR "/";

static bool
read_exact(const char *path, unsigned char *out, size_t len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    bool ok = fread(out, 1, len, f) == len;
    (void)fclose(f);
    return ok;
}

static bool
hash_file(const char *path, crypto_sign_state *state)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;

    unsigned char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0)
        crypto_sign_update(state, buffer, bytes_read);
    bool ok = !ferror(f);
    (void)fclose(f);
    return ok;
}

bool
sign_verify_with_key(const char *pkg_path, const char *sig_path,
                     const char *public_key_path)
{
    if (!pkg_path || !sig_path || !public_key_path)
        return false;
    if (sodium_init() < 0)
        return false;

    unsigned char signature[crypto_sign_BYTES];
    if (!read_exact(sig_path, signature, sizeof(signature)))
        return false;

    unsigned char public_key[crypto_sign_PUBLICKEYBYTES];
    if (!read_exact(public_key_path, public_key, sizeof(public_key)))
        return false;

    crypto_sign_state state;
    crypto_sign_init(&state);
    if (!hash_file(pkg_path, &state))
        return false;

    return crypto_sign_final_verify(&state, signature, public_key) == 0;
}

bool
sign_file_with_key(const char *pkg_path, const char *sig_path,
                   const char *secret_key_path)
{
    if (!pkg_path || !sig_path || !secret_key_path)
        return false;
    if (sodium_init() < 0)
        return false;

    unsigned char secret_key[crypto_sign_SECRETKEYBYTES];
    if (!read_exact(secret_key_path, secret_key, sizeof(secret_key)))
        return false;

    crypto_sign_state state;
    crypto_sign_init(&state);
    if (!hash_file(pkg_path, &state))
        return false;

    unsigned char signature[crypto_sign_BYTES];
    if (crypto_sign_final_create(&state, signature, NULL, secret_key) != 0)
        return false;

    FILE *sig_f = fopen(sig_path, "wb");
    if (!sig_f)
        return false;

    bool ok =
        fwrite(signature, 1, crypto_sign_BYTES, sig_f) == crypto_sign_BYTES;
    if (fclose(sig_f) != 0)
        ok = false;
    return ok;
}

// libsodium uses Ed25519, which is ECC. allow_rsa has no effect here but is
// kept for interface compatibility.
bool
sign_verify(const char *pkg_path, const char *sig_path, bool allow_rsa)
{
    (void)allow_rsa;

    char key_file[PATH_MAX];
    (void)snprintf(key_file, sizeof(key_file), "%spublic.key", key_path);
    return sign_verify_with_key(pkg_path, sig_path, key_file);
}

bool
sign_file(const char *pkg_path, const char *sig_path)
{
    char key_file[PATH_MAX];
    (void)snprintf(key_file, sizeof(key_file), "%ssecret.key", key_path);
    return sign_file_with_key(pkg_path, sig_path, key_file);
}
