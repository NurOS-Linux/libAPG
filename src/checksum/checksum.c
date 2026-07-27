// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../../include/apg/checksum.h"
#include "../../include/apg/crc32.h"
#include "../../include/apg/md5.h"
#include "../../include/apg/sha256.h"
#include "../../include/util.h"

static bool
compute_crc32_file(const char *path, uint32_t *out)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;

    (void)fseek(f, 0, SEEK_END);
    long size = ftell(f);
    (void)fseek(f, 0, SEEK_SET);

    unsigned char *data = malloc((size_t)size);
    if (!data)
    {
        (void)fclose(f);
        return false;
    }

    if (fread(data, 1, (size_t)size, f) != (size_t)size)
    {
        free(data);
        (void)fclose(f);
        return false;
    }
    (void)fclose(f);

    *out = crc32(data, (unsigned int)size);
    free(data);
    return true;
}

static bool
verify_crc32sums(const char *pkg_dir, const char *sums_path)
{
    FILE *f = fopen(sums_path, "r");
    if (!f)
        return false;

    char line[PATH_MAX + 16];
    bool ok = true;

    while (fgets(line, sizeof(line), f))
    {
        char hash_str[9] = {0};
        char rel_path[PATH_MAX];

        char fmt[32];
        (void)snprintf(fmt, sizeof(fmt), "%%8s  %%%zus", sizeof(rel_path) - 1);
        if (sscanf(line, fmt, hash_str, rel_path) != 2)
            continue;

        char *full_path = concat_dirs(pkg_dir, rel_path);
        if (!full_path)
        {
            ok = false;
            break;
        }

        uint32_t computed;
        bool match = compute_crc32_file(full_path, &computed) &&
                     computed == (uint32_t)strtoul(hash_str, NULL, 16);
        free(full_path);

        if (!match)
        {
            ok = false;
            break;
        }
    }

    (void)fclose(f);
    return ok;
}

static bool
verify_md5sums(const char *pkg_dir, const char *sums_path)
{
    FILE *f = fopen(sums_path, "r");
    if (!f)
        return false;

    char line[PATH_MAX + 40];
    bool ok = true;

    while (fgets(line, sizeof(line), f))
    {
        char hash_str[33] = {0};
        char rel_path[PATH_MAX];

        char fmt[32];
        (void)snprintf(fmt, sizeof(fmt), "%%32s  %%%zus", sizeof(rel_path) - 1);
        if (sscanf(line, fmt, hash_str, rel_path) != 2)
            continue;

        char *full_path = concat_dirs(pkg_dir, rel_path);
        if (!full_path)
        {
            ok = false;
            break;
        }

        uint8_t digest[16];
        if (!compute_md5(full_path, digest))
        {
            free(full_path);
            ok = false;
            break;
        }
        free(full_path);

        char computed_str[33];
        for (int i = 0; i < 16; i++)
            (void)snprintf(&computed_str[(size_t)i * 2], 3, "%02x", digest[i]);

        if (strncmp(computed_str, hash_str, 32) != 0)
        {
            ok = false;
            break;
        }
    }

    (void)fclose(f);
    return ok;
}

static bool
verify_sha256sums(const char *pkg_dir, const char *sums_path)
{
    FILE *f = fopen(sums_path, "r");
    if (!f)
        return false;

    char line[PATH_MAX + 68];
    bool ok = true;

    while (fgets(line, sizeof(line), f))
    {
        char hash_str[65] = {0};
        char rel_path[PATH_MAX];

        char fmt[32];
        (void)snprintf(fmt, sizeof(fmt), "%%64s  %%%zus", sizeof(rel_path) - 1);
        if (sscanf(line, fmt, hash_str, rel_path) != 2)
            continue;

        char *full_path = concat_dirs(pkg_dir, rel_path);
        if (!full_path)
        {
            ok = false;
            break;
        }

        uint8_t digest[32];
        if (!compute_sha256(full_path, digest))
        {
            free(full_path);
            ok = false;
            break;
        }
        free(full_path);

        char computed_str[65];
        sha256_hex(digest, computed_str);

        if (strncmp(computed_str, hash_str, 64) != 0)
        {
            ok = false;
            break;
        }
    }

    (void)fclose(f);
    return ok;
}

bool
verify_checksums(const char *pkg_dir)
{
    char path[PATH_MAX];
    FILE *f;

    (void)snprintf(path, sizeof(path), "%s/sha256sums", pkg_dir);
    f = fopen(path, "r");
    if (f)
    {
        (void)fclose(f);
        return verify_sha256sums(pkg_dir, path);
    }

    (void)snprintf(path, sizeof(path), "%s/crc32sums", pkg_dir);
    f = fopen(path, "r");
    if (f)
    {
        (void)fclose(f);
        return verify_crc32sums(pkg_dir, path);
    }

    (void)snprintf(path, sizeof(path), "%s/md5sums", pkg_dir);
    f = fopen(path, "r");
    if (f)
    {
        (void)fclose(f);
        return verify_md5sums(pkg_dir, path);
    }

    return false;
}
