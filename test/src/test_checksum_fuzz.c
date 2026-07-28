// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <apg/checksum.h>
#include <apg/crc32.h>
#include <apg/md5.h>
#include <apg/sha256.h>
#include <util.h>

static char *
mktmp_pkg_dir(const char *suffix)
{
    char tmpl[PATH_MAX];
    snprintf(tmpl, sizeof(tmpl), "/tmp/apg-test-cksum-%s-XXXXXX", suffix);
    char *made = mkdtemp(tmpl);
    assert(made);
    return strdup(made);
}

static void
write_file_at(const char *dir, const char *name, const void *data, size_t len)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    assert(f);
    fwrite(data, 1, len, f);
    fclose(f);
}

static void
remove_file_at(const char *dir, const char *name)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    unlink(path);
}

void
test_verify_checksums_sha256_valid_passes(void)
{
    char *pkg_dir = mktmp_pkg_dir("sha-ok");
    const char *content = "roundtrip payload for sha256";
    write_file_at(pkg_dir, "payload.bin", content, strlen(content));

    char payload_path[PATH_MAX];
    snprintf(payload_path, sizeof(payload_path), "%s/payload.bin", pkg_dir);
    uint8_t digest[32];
    assert(compute_sha256(payload_path, digest));

    char hex[65];
    sha256_hex(digest, hex);

    char line[128];
    snprintf(line, sizeof(line), "%s  payload.bin\n", hex);
    write_file_at(pkg_dir, "sha256sums", line, strlen(line));

    assert(verify_checksums(pkg_dir));

    remove_dir_recursive(pkg_dir);
    free(pkg_dir);
    printf("test_verify_checksums_sha256_valid_passes: PASS\n");
}

void
test_verify_checksums_sha256_tampered_fails(void)
{
    char *pkg_dir = mktmp_pkg_dir("sha-bad");
    const char *content = "original payload";
    write_file_at(pkg_dir, "payload.bin", content, strlen(content));

    char payload_path[PATH_MAX];
    snprintf(payload_path, sizeof(payload_path), "%s/payload.bin", pkg_dir);
    uint8_t digest[32];
    assert(compute_sha256(payload_path, digest));

    char hex[65];
    sha256_hex(digest, hex);
    hex[0] = (hex[0] == '0') ? '1' : '0';

    char line[128];
    snprintf(line, sizeof(line), "%s  payload.bin\n", hex);
    write_file_at(pkg_dir, "sha256sums", line, strlen(line));

    assert(!verify_checksums(pkg_dir));

    remove_dir_recursive(pkg_dir);
    free(pkg_dir);
    printf("test_verify_checksums_sha256_tampered_fails: PASS\n");
}

void
test_verify_checksums_crc32_fallback_passes(void)
{
    char *pkg_dir = mktmp_pkg_dir("crc-ok");
    const char *content = "crc32 fallback payload";
    write_file_at(pkg_dir, "payload.bin", content, strlen(content));

    unsigned int crc =
        crc32((const unsigned char *)content, (unsigned int)strlen(content));

    char line[128];
    snprintf(line, sizeof(line), "%08x  payload.bin\n", crc);
    write_file_at(pkg_dir, "crc32sums", line, strlen(line));

    assert(verify_checksums(pkg_dir));

    remove_dir_recursive(pkg_dir);
    free(pkg_dir);
    printf("test_verify_checksums_crc32_fallback_passes: PASS\n");
}

void
test_verify_checksums_md5_fallback_passes(void)
{
    char *pkg_dir = mktmp_pkg_dir("md5-ok");
    const char *content = "md5 fallback payload";
    write_file_at(pkg_dir, "payload.bin", content, strlen(content));

    md5_ctx ctx;
    uint8_t digest[16];
    md5_init(&ctx);
    md5_update(&ctx, (const uint8_t *)content, strlen(content));
    md5_final(digest, &ctx);

    char hex[33];
    for (int i = 0; i < 16; i++)
        snprintf(&hex[i * 2], 3, "%02x", digest[i]);

    char line[128];
    snprintf(line, sizeof(line), "%s  payload.bin\n", hex);
    write_file_at(pkg_dir, "md5sums", line, strlen(line));

    assert(verify_checksums(pkg_dir));

    remove_dir_recursive(pkg_dir);
    free(pkg_dir);
    printf("test_verify_checksums_md5_fallback_passes: PASS\n");
}

void
test_verify_checksums_missing_sums_file_fails(void)
{
    char *pkg_dir = mktmp_pkg_dir("nosums");
    write_file_at(pkg_dir, "payload.bin", "orphan", 6);

    assert(!verify_checksums(pkg_dir));

    remove_dir_recursive(pkg_dir);
    free(pkg_dir);
    printf("test_verify_checksums_missing_sums_file_fails: PASS\n");
}

void
test_verify_checksums_fuzz_malformed_input_no_crash(void)
{
    char *pkg_dir = mktmp_pkg_dir("fuzz");
    write_file_at(pkg_dir, "payload.bin", "fuzz target payload", 20);

    static const char *names[] = {"sha256sums", "crc32sums", "md5sums"};
    unsigned int seed = 0xF00DBEEFu;
    char buf[8192];

    for (int iter = 0; iter < 2000; iter++)
    {
        const char *name = names[rand_r(&seed) % 3];
        size_t len = (size_t)(rand_r(&seed) % sizeof(buf));

        for (size_t i = 0; i < len; i++)
        {
            unsigned int r = rand_r(&seed) % 6;
            if (r == 0)
                buf[i] = ' ';
            else if (r == 1)
                buf[i] = '\n';
            else if (r == 2)
                buf[i] = (char)("0123456789abcdef"[rand_r(&seed) % 16]);
            else
                buf[i] = (char)(rand_r(&seed) & 0xff);
        }

        write_file_at(pkg_dir, name, buf, len);
        verify_checksums(pkg_dir);
        remove_file_at(pkg_dir, name);
    }

    remove_dir_recursive(pkg_dir);
    free(pkg_dir);
    printf("test_verify_checksums_fuzz_malformed_input_no_crash: PASS\n");
}
