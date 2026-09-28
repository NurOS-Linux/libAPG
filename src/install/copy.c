#include <unistd.h>
// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../../include/apg/copy.h"
#include "../../include/apg/util.h"
#include "../error_priv.h"

bool
copy_file(const char *src, const char *dst)
{
    struct stat st;
    if (stat(src, &st) != 0)
    {
        apg_set_error("cannot read '%s': %s", src, strerror(errno));
        return false;
    }

    FILE *in = fopen(src, "rb");
    if (!in)
    {
        apg_set_error("cannot open '%s': %s", src, strerror(errno));
        return false;
    }

    FILE *out = fopen(dst, "wb");
    if (!out)
    {
        apg_set_error("cannot create '%s': %s", dst, strerror(errno));
        (void)fclose(in);
        return false;
    }

    char buf[4096];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
    {
        if (fwrite(buf, 1, n, out) != n)
        {
            apg_set_error("cannot write '%s': %s", dst, strerror(errno));
            ok = false;
            break;
        }
    }
    if (ok && ferror(in))
    {
        apg_set_error("cannot read '%s': %s", src, strerror(errno));
        ok = false;
    }

    (void)fclose(in);
    if (fclose(out) != 0 && ok)
    {
        apg_set_error("cannot write '%s': %s", dst, strerror(errno));
        ok = false;
    }

    if (ok && chmod(dst, st.st_mode & 07777) != 0)
    {
        apg_set_error("cannot set permissions on '%s': %s", dst,
                      strerror(errno));
        ok = false;
    }

    return ok;
}

bool
copy_dir(const char *src, const char *dst)
{
    struct stat src_st;
    if (stat(src, &src_st) != 0)
    {
        apg_set_error("cannot read '%s': %s", src, strerror(errno));
        return false;
    }

    struct stat dst_st;
    bool dst_existed = stat(dst, &dst_st) == 0;

    if (!dst_existed && mkdir(dst, 0755) != 0 && errno != EEXIST)
    {
        apg_set_error("cannot create directory '%s': %s", dst, strerror(errno));
        return false;
    }

    if (!dst_existed && chmod(dst, src_st.st_mode & 07777) != 0)
    {
        apg_set_error("cannot set permissions on '%s': %s", dst,
                      strerror(errno));
        return false;
    }

    DIR *dir = opendir(src);
    if (!dir)
    {
        apg_set_error("cannot open directory '%s': %s", src, strerror(errno));
        return false;
    }

    struct dirent *entry;
    bool ok = true;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        char *src_path = concat_dirs(src, entry->d_name);
        char *dst_path = concat_dirs(dst, entry->d_name);

        if (!src_path || !dst_path)
        {
            apg_set_error("out of memory while copying '%s'", src);
            free(src_path);
            free(dst_path);
            ok = false;
            break;
        }

        struct stat st;
        if (lstat(src_path, &st) != 0)
        {
            apg_set_error("cannot read '%s': %s", src_path, strerror(errno));
            free(src_path);
            free(dst_path);
            ok = false;
            break;
        }

        if (S_ISDIR(st.st_mode))
        {
            struct stat dst_item_st;
            if (stat(dst_path, &dst_item_st) != 0 ||
                !S_ISDIR(dst_item_st.st_mode))
            {
                unlink(dst_path);
            }
            ok = copy_dir(src_path, dst_path);
        }
        else if (S_ISLNK(st.st_mode))
        {
            char target[PATH_MAX];
            ssize_t len = readlink(src_path, target, sizeof(target) - 1);
            if (len < 0)
            {
                apg_set_error("cannot read link '%s': %s", src_path,
                              strerror(errno));
                free(src_path);
                free(dst_path);
                ok = false;
                break;
            }
            target[len] = '\0';

            struct stat dst_item_st;
            if (lstat(dst_path, &dst_item_st) == 0)
            {
                if (S_ISDIR(dst_item_st.st_mode))
                    remove_dir_recursive(dst_path);
                else
                    unlink(dst_path);
            }

            if (symlink(target, dst_path) != 0)
            {
                apg_set_error("cannot create link '%s': %s", dst_path,
                              strerror(errno));
                free(src_path);
                free(dst_path);
                ok = false;
                break;
            }
            ok = true;
        }
        else if (S_ISREG(st.st_mode))
        {
            struct stat dst_item_st;
            if (lstat(dst_path, &dst_item_st) == 0)
            {
                if (S_ISDIR(dst_item_st.st_mode))
                    remove_dir_recursive(dst_path);
                else
                    unlink(dst_path);
            }
            ok = copy_file(src_path, dst_path);
        }

        free(src_path);
        free(dst_path);

        if (!ok)
            break;
    }

    closedir(dir);
    return ok;
}
