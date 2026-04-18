// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

#include "../../include/apg/scripts.h"
#include "../../include/util.h"

static void
normalize(const char *src, char *dst, size_t dst_size)
{
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < dst_size; i++) {
        if (src[i] == '-' || src[i] == '_')
            continue;
        dst[j++] = (char)tolower((unsigned char)src[i]);
    }
    dst[j] = '\0';
}

static bool
exec_script(const char *path)
{
    pid_t pid = fork();
    if (pid < 0) return false;

    if (pid == 0) {
        execl(path, path, (char *)NULL);
        _exit(1);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) return false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool
run_script(const char *pkg_dir, const char *name)
{
    char *scripts_dir = concat_dirs(pkg_dir, "scripts");
    if (!scripts_dir) return false;

    DIR *dir = opendir(scripts_dir);
    if (!dir) {
        free(scripts_dir);
        return true;
    }

    char name_norm[256];
    normalize(name, name_norm, sizeof(name_norm));

    char *found = NULL;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char entry_norm[256];
        normalize(entry->d_name, entry_norm, sizeof(entry_norm));

        if (strcmp(entry_norm, name_norm) == 0) {
            found = concat_dirs(scripts_dir, entry->d_name);
            break;
        }
    }

    closedir(dir);
    free(scripts_dir);

    if (!found) return true;

    if (access(found, X_OK) != 0) {
        free(found);
        return true;
    }

    bool ok = exec_script(found);
    free(found);
    return ok;
}
