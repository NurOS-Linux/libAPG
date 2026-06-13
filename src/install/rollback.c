// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../../include/apg/install.h"
#include "../../include/util.h"

// Mirror the src directory tree and remove corresponding files under dst.
// Directories are removed with rmdir after their contents — rmdir silently
// fails on non-empty dirs, which is intentional: we only clean up what we put
// there.
static void
remove_mirrored(const char *src, const char *dst)
{
    DIR *dir = opendir(src);
    if (!dir)
        return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        char *src_path = concat_dirs(src, entry->d_name);
        char *dst_path = concat_dirs(dst, entry->d_name);
        if (!src_path || !dst_path)
        {
            free(src_path);
            free(dst_path);
            break;
        }

        struct stat st;
        if (stat(src_path, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
                remove_mirrored(src_path, dst_path);
            else if (S_ISREG(st.st_mode))
                unlink(dst_path);
        }

        free(src_path);
        free(dst_path);
    }

    closedir(dir);
    rmdir(dst);
}

void
rollback_install(const char *pkg_dir, const char *root_path)
{
    char *data_src = concat_dirs(pkg_dir, "data");
    if (data_src)
    {
        struct stat st;
        if (stat(data_src, &st) == 0 && S_ISDIR(st.st_mode))
            remove_mirrored(data_src, root_path);
        free(data_src);
    }

    const char *home = getenv("HOME");
    if (!home)
        return;

    char *home_src = concat_dirs(pkg_dir, "home");
    if (home_src)
    {
        struct stat st;
        if (stat(home_src, &st) == 0 && S_ISDIR(st.st_mode))
            remove_mirrored(home_src, home);
        free(home_src);
    }
}
