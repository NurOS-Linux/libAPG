// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sodium.h>

#include <apg/keyring.h>
#include <apg/sign.h>

#include "fs_helpers.h"

struct keypair
{
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    unsigned char sk[crypto_sign_SECRETKEYBYTES];
};

static void
write_bytes(const char *path, const unsigned char *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    assert(f);
    assert(fwrite(data, 1, len, f) == len);
    fclose(f);
}

static void
sign_stream(const unsigned char *sk, const char *pkg_path, const char *sig_path)
{
    crypto_sign_state st;
    crypto_sign_init(&st);
    FILE *f = fopen(pkg_path, "rb");
    assert(f);
    unsigned char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        crypto_sign_update(&st, buf, n);
    fclose(f);
    unsigned char sig[crypto_sign_BYTES];
    assert(crypto_sign_final_create(&st, sig, NULL, sk) == 0);
    write_bytes(sig_path, sig, sizeof(sig));
}

static char *
make_keyring_dir(const struct keypair *keys, int count)
{
    char *dir = fs_mktmp("keyring");
    for (int i = 0; i < count; i++)
    {
        char name[32];
        snprintf(name, sizeof(name), "key%d.key", i);
        char *p = fs_join(dir, name);
        write_bytes(p, keys[i].pk, sizeof(keys[i].pk));
        free(p);
    }
    return dir;
}

void
test_keyring_load_handles_directory_contents(void)
{
    assert(sodium_init() >= 0);
    assert(keyring_load("/nonexistent-apg-test-dir") == NULL);

    struct keypair kp[2];
    crypto_sign_keypair(kp[0].pk, kp[0].sk);
    crypto_sign_keypair(kp[1].pk, kp[1].sk);

    char *dir = fs_mktmp("kr-load");
    char *pkg = fs_join(dir, "pkg.bin");
    char *sig = fs_join(dir, "pkg.sig");
    fs_write(pkg, "package bytes");
    sign_stream(kp[1].sk, pkg, sig);

    struct keyring *empty = keyring_load(dir);
    assert(empty);
    assert(!keyring_verify(empty, pkg, sig));
    keyring_free(empty);

    char *short_key = fs_join(dir, "short.key");
    write_bytes(short_key, kp[1].pk, 16);
    char *wrong_ext = fs_join(dir, "good.pub");
    write_bytes(wrong_ext, kp[1].pk, sizeof(kp[1].pk));
    char *no_ext = fs_join(dir, "keyfile");
    write_bytes(no_ext, kp[1].pk, sizeof(kp[1].pk));
    struct keyring *kr = keyring_load(dir);
    assert(kr);
    assert(!keyring_verify(kr, pkg, sig));
    keyring_free(kr);

    char *k0 = fs_join(dir, "a.key");
    char *k1 = fs_join(dir, "b.key");
    write_bytes(k0, kp[0].pk, sizeof(kp[0].pk));
    write_bytes(k1, kp[1].pk, sizeof(kp[1].pk));
    kr = keyring_load(dir);
    assert(kr);
    assert(keyring_verify(kr, pkg, sig));
    keyring_free(kr);

    keyring_free(NULL);

    free(pkg);
    free(sig);
    free(short_key);
    free(wrong_ext);
    free(no_ext);
    free(k0);
    free(k1);
    fs_rmtree(dir);
    free(dir);
    printf("test_keyring_load_handles_directory_contents: PASS\n");
}

void
test_keyring_verify_accepts_only_valid_signatures(void)
{
    assert(sodium_init() >= 0);
    struct keypair kp[3];
    for (int i = 0; i < 3; i++)
        crypto_sign_keypair(kp[i].pk, kp[i].sk);

    char *kdir = make_keyring_dir(kp, 2);
    struct keyring *kr = keyring_load(kdir);
    assert(kr);

    char *dir = fs_mktmp("kr-verify");
    char *pkg = fs_join(dir, "pkg.bin");
    char *sig = fs_join(dir, "pkg.sig");
    fs_write(pkg, "original content");

    sign_stream(kp[0].sk, pkg, sig);
    assert(keyring_verify(kr, pkg, sig));
    sign_stream(kp[1].sk, pkg, sig);
    assert(keyring_verify(kr, pkg, sig));

    sign_stream(kp[2].sk, pkg, sig);
    assert(!keyring_verify(kr, pkg, sig));

    sign_stream(kp[0].sk, pkg, sig);
    fs_write(pkg, "tampered content");
    assert(!keyring_verify(kr, pkg, sig));

    fs_write(pkg, "original content");
    assert(keyring_verify(kr, pkg, sig));

    unsigned char raw[crypto_sign_BYTES];
    FILE *f = fopen(sig, "rb");
    assert(f && fread(raw, 1, sizeof(raw), f) == sizeof(raw));
    fclose(f);

    write_bytes(sig, raw, sizeof(raw) - 1);
    assert(!keyring_verify(kr, pkg, sig));
    write_bytes(sig, raw, 0);
    assert(!keyring_verify(kr, pkg, sig));
    raw[10] ^= 0x01;
    write_bytes(sig, raw, sizeof(raw));
    assert(!keyring_verify(kr, pkg, sig));

    char *missing = fs_join(dir, "missing");
    assert(!keyring_verify(kr, pkg, missing));
    assert(!keyring_verify(kr, missing, sig));
    assert(!keyring_verify(NULL, pkg, sig));
    assert(!keyring_verify(kr, NULL, sig));
    assert(!keyring_verify(kr, pkg, NULL));

    free(missing);
    free(pkg);
    free(sig);
    keyring_free(kr);
    fs_rmtree(dir);
    fs_rmtree(kdir);
    free(dir);
    free(kdir);
    printf("test_keyring_verify_accepts_only_valid_signatures: PASS\n");
}

void
test_keyring_verify_large_and_empty_packages(void)
{
    assert(sodium_init() >= 0);
    struct keypair kp;
    crypto_sign_keypair(kp.pk, kp.sk);
    char *kdir = make_keyring_dir(&kp, 1);
    struct keyring *kr = keyring_load(kdir);
    assert(kr);

    char *dir = fs_mktmp("kr-big");
    char *pkg = fs_join(dir, "pkg.bin");
    char *sig = fs_join(dir, "pkg.sig");

    fs_write(pkg, "");
    sign_stream(kp.sk, pkg, sig);
    assert(keyring_verify(kr, pkg, sig));

    FILE *f = fopen(pkg, "wb");
    assert(f);
    for (int i = 0; i < 5000; i++)
        fprintf(f, "block-%06d-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n", i);
    fclose(f);
    sign_stream(kp.sk, pkg, sig);
    assert(keyring_verify(kr, pkg, sig));

    f = fopen(pkg, "ab");
    assert(f);
    fputc('x', f);
    fclose(f);
    assert(!keyring_verify(kr, pkg, sig));

    free(pkg);
    free(sig);
    keyring_free(kr);
    fs_rmtree(dir);
    fs_rmtree(kdir);
    free(dir);
    free(kdir);
    printf("test_keyring_verify_large_and_empty_packages: PASS\n");
}

void
test_keyring_add_key_requires_endorsement(void)
{
    assert(sodium_init() >= 0);
    struct keypair root, newcomer, stranger;
    crypto_sign_keypair(root.pk, root.sk);
    crypto_sign_keypair(newcomer.pk, newcomer.sk);
    crypto_sign_keypair(stranger.pk, stranger.sk);

    char *kdir = make_keyring_dir(&root, 1);
    struct keyring *trusted = keyring_load(kdir);
    assert(trusted);

    char *work = fs_mktmp("kr-add");
    char *new_key = fs_join(work, "newcomer.key");
    char *good_sig = fs_join(work, "good.sig");
    char *bad_sig = fs_join(work, "bad.sig");
    char *pkg = fs_join(work, "pkg.bin");
    char *pkg_sig = fs_join(work, "pkg.sig");
    write_bytes(new_key, newcomer.pk, sizeof(newcomer.pk));
    fs_write(pkg, "signed by newcomer");
    sign_stream(newcomer.sk, pkg, pkg_sig);

    unsigned char sig[crypto_sign_BYTES];
    crypto_sign_detached(sig, NULL, newcomer.pk, sizeof(newcomer.pk),
                         stranger.sk);
    write_bytes(bad_sig, sig, sizeof(sig));
    assert(!keyring_add_key(kdir, new_key, bad_sig, trusted));

    struct keyring *kr = keyring_load(kdir);
    assert(kr);
    assert(!keyring_verify(kr, pkg, pkg_sig));
    keyring_free(kr);

    crypto_sign_detached(sig, NULL, newcomer.pk, sizeof(newcomer.pk), root.sk);
    write_bytes(good_sig, sig, sizeof(sig));
    assert(keyring_add_key(kdir, new_key, good_sig, trusted));

    kr = keyring_load(kdir);
    assert(kr);
    assert(keyring_verify(kr, pkg, pkg_sig));
    keyring_free(kr);

    assert(!keyring_add_key(NULL, new_key, good_sig, trusted));
    assert(!keyring_add_key(kdir, NULL, good_sig, trusted));
    assert(!keyring_add_key(kdir, new_key, NULL, trusted));
    assert(!keyring_add_key(kdir, new_key, good_sig, NULL));

    char *missing = fs_join(work, "missing");
    assert(!keyring_add_key(kdir, missing, good_sig, trusted));
    assert(!keyring_add_key(kdir, new_key, missing, trusted));
    assert(!keyring_add_key("/nonexistent-apg-test-dir", new_key, good_sig,
                            trusted));

    char *short_key = fs_join(work, "short.key");
    write_bytes(short_key, newcomer.pk, 8);
    assert(!keyring_add_key(kdir, short_key, good_sig, trusted));

    char *short_sig = fs_join(work, "short.sig");
    write_bytes(short_sig, sig, 10);
    assert(!keyring_add_key(kdir, new_key, short_sig, trusted));

    free(missing);
    free(short_key);
    free(short_sig);
    free(new_key);
    free(good_sig);
    free(bad_sig);
    free(pkg);
    free(pkg_sig);
    keyring_free(trusted);
    fs_rmtree(work);
    fs_rmtree(kdir);
    free(work);
    free(kdir);
    printf("test_keyring_add_key_requires_endorsement: PASS\n");
}

void
test_keyring_add_key_with_any_filename_is_loadable(void)
{
    assert(sodium_init() >= 0);
    struct keypair root, newcomer;
    crypto_sign_keypair(root.pk, root.sk);
    crypto_sign_keypair(newcomer.pk, newcomer.sk);

    char *kdir = make_keyring_dir(&root, 1);
    struct keyring *trusted = keyring_load(kdir);
    assert(trusted);

    char *work = fs_mktmp("kr-name");
    char *new_key = fs_join(work, "vendor-public");
    char *key_sig = fs_join(work, "vendor.sig");
    char *pkg = fs_join(work, "pkg.bin");
    char *pkg_sig = fs_join(work, "pkg.sig");
    write_bytes(new_key, newcomer.pk, sizeof(newcomer.pk));
    fs_write(pkg, "vendor package");
    sign_stream(newcomer.sk, pkg, pkg_sig);

    unsigned char sig[crypto_sign_BYTES];
    crypto_sign_detached(sig, NULL, newcomer.pk, sizeof(newcomer.pk), root.sk);
    write_bytes(key_sig, sig, sizeof(sig));

    bool added = keyring_add_key(kdir, new_key, key_sig, trusted);
    struct keyring *kr = keyring_load(kdir);
    assert(kr);
    bool usable = keyring_verify(kr, pkg, pkg_sig);
    keyring_free(kr);

    assert(added == usable);

    free(new_key);
    free(key_sig);
    free(pkg);
    free(pkg_sig);
    keyring_free(trusted);
    fs_rmtree(work);
    fs_rmtree(kdir);
    free(work);
    free(kdir);
    printf("test_keyring_add_key_with_any_filename_is_loadable: PASS\n");
}

static bool
keys_dir_is_usable(void)
{
    char *pub = fs_join(APG_KEYS_DIR, "public.key");
    char *sec = fs_join(APG_KEYS_DIR, "secret.key");
    bool free_slot = access(pub, F_OK) != 0 && access(sec, F_OK) != 0;
    bool writable = access(APG_KEYS_DIR, W_OK) == 0;
    free(pub);
    free(sec);
    return free_slot && writable;
}

void
test_sign_file_and_verify_roundtrip(void)
{
    if (!keys_dir_is_usable())
    {
        printf("test_sign_file_and_verify_roundtrip: SKIP (%s not usable)\n",
               APG_KEYS_DIR);
        return;
    }
    assert(sodium_init() >= 0);

    struct keypair kp, other;
    crypto_sign_keypair(kp.pk, kp.sk);
    crypto_sign_keypair(other.pk, other.sk);

    char *pub = fs_join(APG_KEYS_DIR, "public.key");
    char *sec = fs_join(APG_KEYS_DIR, "secret.key");
    write_bytes(pub, kp.pk, sizeof(kp.pk));
    write_bytes(sec, kp.sk, sizeof(kp.sk));

    char *work = fs_mktmp("sign");
    char *pkg = fs_join(work, "pkg.bin");
    char *sig = fs_join(work, "pkg.sig");
    fs_write(pkg, "content to sign");

    assert(sign_file(pkg, sig));
    assert(sign_verify(pkg, sig, false));
    assert(sign_verify(pkg, sig, true));

    char *kdir = make_keyring_dir(&kp, 1);
    struct keyring *kr = keyring_load(kdir);
    assert(kr);
    assert(keyring_verify(kr, pkg, sig));
    keyring_free(kr);

    fs_write(pkg, "content to sign!");
    assert(!sign_verify(pkg, sig, false));
    fs_write(pkg, "content to sign");
    assert(sign_verify(pkg, sig, false));

    char *missing = fs_join(work, "missing");
    assert(!sign_verify(missing, sig, false));
    assert(!sign_verify(pkg, missing, false));
    assert(!sign_file(missing, sig));
    char *bad_out = fs_join(work, "nodir/out.sig");
    assert(!sign_file(pkg, bad_out));

    char *short_sig = fs_join(work, "short.sig");
    write_bytes(short_sig, (const unsigned char *)"abc", 3);
    assert(!sign_verify(pkg, short_sig, false));

    sign_stream(other.sk, pkg, sig);
    assert(!sign_verify(pkg, sig, false));

    unlink(pub);
    assert(!sign_verify(pkg, sig, false));
    unlink(sec);
    assert(!sign_file(pkg, sig));

    free(missing);
    free(bad_out);
    free(short_sig);
    free(pkg);
    free(sig);
    free(pub);
    free(sec);
    fs_rmtree(work);
    fs_rmtree(kdir);
    free(work);
    free(kdir);
    printf("test_sign_file_and_verify_roundtrip: PASS\n");
}
