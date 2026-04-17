// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <sys/stat.h>

#include "../../include/apg/install.h"
#include "../../include/apg/copy.h"
#include "../../include/util.h"

bool
install_data_dir(const char *pkg_dir, const char *root_path)
{
    char *data_src = concat_dirs(pkg_dir, "data");
    if (!data_src) return false;

    struct stat st;
    if (stat(data_src, &st) != 0 || !S_ISDIR(st.st_mode)) {
        free(data_src);
        return false;
    }

    bool ok = copy_dir(data_src, root_path);
    free(data_src);
    return ok;
}

bool
install_home_dir(const char *pkg_dir)
{
    const char *home = getenv("HOME");
    if (!home) return false;

    char *home_src = concat_dirs(pkg_dir, "home");
    if (!home_src) return false;

    struct stat st;
    if (stat(home_src, &st) != 0 || !S_ISDIR(st.st_mode)) {
        free(home_src);
        return true;
    }

    bool ok = copy_dir(home_src, home);
    free(home_src);
    return ok;
}
