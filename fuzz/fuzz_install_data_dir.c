// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/install.h>
#include <apg/copy.h>
#include <util.h>

#define FUZZ_MAX_ENTRIES 6
#define FUZZ_NAME_POOL_SIZE 5

static const char *
pool_name(uint8_t idx)
{
    static const char *names[FUZZ_NAME_POOL_SIZE] = {
        "aa", "bb", "cc", "dd", "ee",
    };
    return names[idx % FUZZ_NAME_POOL_SIZE];
}

static void
mkdir_recursive(const char *path)
{
    char buf[PATH_MAX];
    size_t len = strlen(path);
    if (len >= sizeof(buf))
        return;
    memcpy(buf, path, len + 1);

    for (size_t i = 1; i < len; i++)
    {
        if (buf[i] != '/')
            continue;
        buf[i] = '\0';
        mkdir(buf, 0755);
        buf[i] = '/';
    }
    mkdir(buf, 0755);
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 1)
        return 0;

    char pkg_dir[] = "/tmp/apg-fuzz-pkg-XXXXXX";
    char root_dir[] = "/tmp/apg-fuzz-root-XXXXXX";
    if (!mkdtemp(pkg_dir) || !mkdtemp(root_dir))
        return 0;

    char data_dir[PATH_MAX];
    snprintf(data_dir, sizeof(data_dir), "%s/data", pkg_dir);
    mkdir_recursive(data_dir);

    int entry_count = data[0] % FUZZ_MAX_ENTRIES;
    size_t pos = 1;

    for (int i = 0; i < entry_count && pos < size; i++)
    {
        uint8_t depth = pos < size ? data[pos++] % 3 : 0;
        char rel[PATH_MAX] = {0};
        size_t rel_len = 0;
        for (uint8_t d = 0; d <= depth && pos < size; d++)
        {
            const char *seg = pool_name(data[pos++]);
            int n = snprintf(rel + rel_len, sizeof(rel) - rel_len, "%s%s",
                             d > 0 ? "/" : "", seg);
            if (n < 0 || (size_t)n >= sizeof(rel) - rel_len)
                break;
            rel_len += (size_t)n;
        }

        char full_dir[PATH_MAX];
        snprintf(full_dir, sizeof(full_dir), "%s/%s", data_dir, rel);
        char *last_slash = strrchr(full_dir, '/');
        if (last_slash && last_slash != full_dir)
        {
            *last_slash = '\0';
            mkdir_recursive(full_dir);
            *last_slash = '/';
        }

        int fd = open(full_dir, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0)
        {
            if (pos < size)
            {
                size_t wlen = data[pos] % 32;
                pos++;
                if (pos + wlen > size)
                    wlen = size > pos ? size - pos : 0;
                if (wlen > 0)
                    (void)!write(fd, data + pos, wlen);
                pos += wlen;
            }
            if (pos < size)
                fchmod(fd, 0400 | (data[pos++] & 0377));
            close(fd);
        }
    }

    (void)install_data_dir(pkg_dir, root_dir);

    remove_dir_recursive(pkg_dir);
    remove_dir_recursive(root_dir);

    return 0;
}
