// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/keyring.h>
#include <apg/util.h>

#define KEY_LEN 32
#define SIG_LEN 64
#define FLAG_ADD_KEY 0x04

static char g_dir[] = "/tmp/apg-fuzz-keyring-XXXXXX";
static char g_keys[PATH_MAX];
static char g_pkg[PATH_MAX];
static char g_sig[PATH_MAX];
static char g_new_key[PATH_MAX];

static void
cleanup(void)
{
    remove_dir_recursive(g_dir);
}

static void
write_bytes(const char *path, const uint8_t *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    if (len > 0)
        (void)!fwrite(data, 1, len, f);
    fclose(f);
}

static size_t
take(const uint8_t *data, size_t size, size_t *pos, size_t want)
{
    size_t left = size - *pos;
    size_t n = want < left ? want : left;
    *pos += n;
    return n;
}

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    if (!mkdtemp(g_dir))
        abort();
    snprintf(g_keys, sizeof(g_keys), "%s/keys", g_dir);
    snprintf(g_pkg, sizeof(g_pkg), "%s/pkg.bin", g_dir);
    snprintf(g_sig, sizeof(g_sig), "%s/pkg.sig", g_dir);
    snprintf(g_new_key, sizeof(g_new_key), "%s/new.key", g_dir);
    atexit(cleanup);
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 1)
        return 0;

    remove_dir_recursive(g_keys);
    mkdir(g_keys, 0755);

    uint8_t flags = data[0];
    size_t pos = 1;

    int key_count = flags & 0x03;

    for (int i = 0; i < key_count; i++)
    {
        size_t start = pos;
        size_t n = take(data, size, &pos, KEY_LEN);
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/k%d.key", g_keys, i);
        write_bytes(path, data + start, n);
    }

    size_t sig_start = pos;
    size_t sig_n = take(data, size, &pos, SIG_LEN);
    write_bytes(g_sig, data + sig_start, sig_n);

    const uint8_t *pkg = data + pos;
    size_t pkg_len = size - pos;
    write_bytes(g_pkg, pkg, pkg_len);

    struct keyring *kr = keyring_load(g_keys);
    if (!kr)
        return 0;

    (void)keyring_verify(kr, g_pkg, g_sig);

    if (flags & FLAG_ADD_KEY)
    {
        write_bytes(g_new_key, pkg, pkg_len < KEY_LEN ? pkg_len : KEY_LEN);
        (void)keyring_add_key(g_keys, g_new_key, g_sig, kr);
    }

    keyring_free(kr);
    return 0;
}
