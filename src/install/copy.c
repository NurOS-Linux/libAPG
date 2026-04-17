// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../../include/apg/copy.h"
#include "../../include/util.h"

static bool
copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb");
    if (!in) return false;

    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return false; }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);

    fclose(in);
    fclose(out);
    return true;
}

bool
copy_dir(const char *src, const char *dst)
{
    create_dir(dst);

    DIR *dir = opendir(src);
    if (!dir) return false;

    struct dirent *entry;
    bool ok = true;

    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        char *src_path = concat_dirs(src, entry->d_name);
        char *dst_path = concat_dirs(dst, entry->d_name);

        if (!src_path || !dst_path) {
            free(src_path);
            free(dst_path);
            ok = false;
            break;
        }

        struct stat st;
        if (stat(src_path, &st) != 0) {
            free(src_path);
            free(dst_path);
            ok = false;
            break;
        }

        if (S_ISDIR(st.st_mode))
            ok = copy_dir(src_path, dst_path);
        else if (S_ISREG(st.st_mode))
            ok = copy_file(src_path, dst_path);

        free(src_path);
        free(dst_path);

        if (!ok) break;
    }

    closedir(dir);
    return ok;
}
