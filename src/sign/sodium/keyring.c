// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sodium.h>

#include "../../../include/apg/keyring.h"

#define KEY_BYTES crypto_sign_PUBLICKEYBYTES

struct keyring
{
    unsigned char (*keys)[KEY_BYTES];
    size_t count;
    size_t cap;
};

static bool
push_key(struct keyring *kr, const unsigned char key[KEY_BYTES])
{
    if (kr->count == kr->cap)
    {
        size_t new_cap = kr->cap ? kr->cap * 2 : 8;
        void *tmp = realloc(kr->keys, new_cap * KEY_BYTES);
        if (!tmp)
            return false;
        kr->keys = tmp;
        kr->cap = new_cap;
    }
    memcpy(kr->keys[kr->count++], key, KEY_BYTES);
    return true;
}

static bool
has_key_ext(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && strcmp(dot, ".key") == 0;
}

struct keyring *
keyring_load(const char *keyring_dir)
{
    if (sodium_init() < 0)
        return NULL;

    struct keyring *kr = calloc(1, sizeof(*kr));
    if (!kr)
        return NULL;

    DIR *dir = opendir(keyring_dir);
    if (!dir)
    {
        free(kr);
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!has_key_ext(entry->d_name))
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", keyring_dir, entry->d_name);

        FILE *f = fopen(path, "rb");
        if (!f)
            continue;

        unsigned char key[KEY_BYTES];
        if (fread(key, 1, KEY_BYTES, f) == KEY_BYTES)
            push_key(kr, key);
        fclose(f);
    }
    closedir(dir);
    return kr;
}

void
keyring_free(struct keyring *kr)
{
    if (!kr)
        return;
    free(kr->keys);
    free(kr);
}

bool
keyring_verify(const struct keyring *kr, const char *pkg_path,
               const char *sig_path)
{
    if (!kr || !pkg_path || !sig_path || kr->count == 0)
        return false;

    unsigned char sig[crypto_sign_BYTES];
    FILE *sf = fopen(sig_path, "rb");
    if (!sf)
        return false;
    bool ok = fread(sig, 1, crypto_sign_BYTES, sf) == crypto_sign_BYTES;
    fclose(sf);
    if (!ok)
        return false;

    crypto_sign_state base;
    crypto_sign_init(&base);

    FILE *pf = fopen(pkg_path, "rb");
    if (!pf)
        return false;
    unsigned char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pf)) > 0)
        crypto_sign_update(&base, buf, n);
    fclose(pf);

    for (size_t i = 0; i < kr->count; i++)
    {
        crypto_sign_state copy = base;
        if (crypto_sign_final_verify(&copy, sig, kr->keys[i]) == 0)
            return true;
    }
    return false;
}

bool
keyring_add_key(const char *keyring_dir, const char *new_key_path,
                const char *key_sig_path, const struct keyring *trusted)
{
    if (!keyring_dir || !new_key_path || !key_sig_path || !trusted)
        return false;
    if (sodium_init() < 0)
        return false;

    unsigned char new_key[KEY_BYTES];
    FILE *kf = fopen(new_key_path, "rb");
    if (!kf)
        return false;
    bool ok = fread(new_key, 1, KEY_BYTES, kf) == KEY_BYTES;
    fclose(kf);
    if (!ok)
        return false;

    unsigned char key_sig[crypto_sign_BYTES];
    FILE *sf = fopen(key_sig_path, "rb");
    if (!sf)
        return false;
    ok = fread(key_sig, 1, crypto_sign_BYTES, sf) == crypto_sign_BYTES;
    fclose(sf);
    if (!ok)
        return false;

    // The new key bytes must be signed by an existing trusted key
    bool endorsed = false;
    for (size_t i = 0; i < trusted->count && !endorsed; i++)
    {
        if (crypto_sign_verify_detached(key_sig, new_key, KEY_BYTES,
                                        trusted->keys[i]) == 0)
            endorsed = true;
    }
    if (!endorsed)
        return false;

    const char *base = strrchr(new_key_path, '/');
    base = base ? base + 1 : new_key_path;

    char dst[PATH_MAX];
    snprintf(dst, sizeof(dst), "%s/%s", keyring_dir, base);

    FILE *df = fopen(dst, "wb");
    if (!df)
        return false;
    ok = fwrite(new_key, 1, KEY_BYTES, df) == KEY_BYTES;
    fclose(df);
    return ok;
}
