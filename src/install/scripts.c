// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <ctype.h>
#include <dirent.h>
#include <inttypes.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __linux__
#include <sched.h>
#endif

#include "../../include/apg/scripts.h"
#include "../../include/apg/copy.h"
#include "../../include/util.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void
normalize(const char *src, char *dst, size_t dst_size)
{
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < dst_size; i++)
    {
        if (src[i] == '-' || src[i] == '_')
            continue;
        dst[j++] = (char)tolower((unsigned char)src[i]);
    }
    dst[j] = '\0';
}

static _Atomic uint64_t g_script_seq = 0;

static bool
exec_script(const char *path, const char *root_path)
{
    bool do_chroot = (root_path != NULL && strcmp(root_path, "/") != 0 &&
                      *root_path != '\0');
    char *exec_path = NULL;
    char *stage_full_path = NULL;

    if (do_chroot)
    {
        size_t root_len = strlen(root_path);
        while (root_len > 1 && root_path[root_len - 1] == '/')
            root_len--;

        if (strncmp(path, root_path, root_len) == 0 &&
            (path[root_len] == '/' || path[root_len] == '\0'))
        {
            const char *rel = path + root_len;
            if (*rel == '\0')
                exec_path = strdup("/");
            else
                exec_path = strdup(rel);
        }
        else
        {
            uint64_t seq = ++g_script_seq;
            pid_t pid = getpid();

            char stage_dir[PATH_MAX];
            (void)snprintf(stage_dir, sizeof(stage_dir), "%.*s/tmp",
                           (int)root_len, root_path);
            create_dir(stage_dir);

            char stage_path[PATH_MAX];
            (void)snprintf(stage_path, sizeof(stage_path),
                           "%.*s/tmp/.apg_script_%d_%" PRIu64, (int)root_len,
                           root_path, (int)pid, seq);

            bool cpy_ok = copy_file(path, stage_path);
            if (cpy_ok)
                chmod(stage_path, 0755);

            if (!cpy_ok)
                return false;

            stage_full_path = strdup(stage_path);

            char exec_buf[128];
            (void)snprintf(exec_buf, sizeof(exec_buf),
                           "/tmp/.apg_script_%d_%" PRIu64, (int)pid, seq);
            exec_path = strdup(exec_buf);
        }

        if (!exec_path)
        {
            if (stage_full_path)
            {
                unlink(stage_full_path);
                free(stage_full_path);
            }
            return false;
        }
    }
    else
    {
        exec_path = strdup(path);
        if (!exec_path)
            return false;
    }

#ifdef __linux__
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        if (stage_full_path)
        {
            unlink(stage_full_path);
            free(stage_full_path);
        }
        free(exec_path);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        if (stage_full_path)
        {
            unlink(stage_full_path);
            free(stage_full_path);
        }
        free(exec_path);
        return false;
    }

    if (pid == 0)
    {
        close(pipefd[0]);

        if (unshare(CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWUTS |
                    CLONE_NEWIPC) < 0)
        {
            uint8_t err = 1;
            (void)write(pipefd[1], &err, 1);
            close(pipefd[1]);
            _exit(1);
        }

        if (do_chroot)
        {
            if (chroot(root_path) < 0 || chdir("/") < 0)
            {
                uint8_t err = 1;
                (void)write(pipefd[1], &err, 1);
                close(pipefd[1]);
                _exit(1);
            }
        }

        close(pipefd[1]);
        execl(exec_path, exec_path, (char *)NULL);
        perror("execl failed");
        _exit(1);
    }

    close(pipefd[1]);
    uint8_t err = 0;
    ssize_t n = read(pipefd[0], &err, 1);
    close(pipefd[0]);

    bool success = false;
    int status;
    if (n == 0 && waitpid(pid, &status, 0) == pid)
    {
        success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
    else if (n > 0)
    {
        waitpid(pid, NULL, 0);
    }

    if (stage_full_path)
    {
        unlink(stage_full_path);
        free(stage_full_path);
    }
    free(exec_path);
    return success;

#else
    // fexecve() can't run #! scripts on FreeBSD (no pathname for the
    // interpreter's argv), so this execs by path instead, like execl().
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        if (stage_full_path)
        {
            unlink(stage_full_path);
            free(stage_full_path);
        }
        free(exec_path);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        if (stage_full_path)
        {
            unlink(stage_full_path);
            free(stage_full_path);
        }
        free(exec_path);
        return false;
    }

    if (pid == 0)
    {
        close(pipefd[0]);

        if (do_chroot)
        {
            if (chroot(root_path) < 0 || chdir("/") < 0)
            {
                uint8_t err = 1;
                (void)write(pipefd[1], &err, 1);
                close(pipefd[1]);
                _exit(1);
            }
        }

        close(pipefd[1]);
        execl(exec_path, exec_path, (char *)NULL);
        _exit(1);
    }

    close(pipefd[1]);
    uint8_t err = 0;
    ssize_t n = read(pipefd[0], &err, 1);
    close(pipefd[0]);

    bool success = false;
    int status;
    if (n == 0 && waitpid(pid, &status, 0) == pid)
    {
        success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
    else if (n > 0)
    {
        waitpid(pid, NULL, 0);
    }

    if (stage_full_path)
    {
        unlink(stage_full_path);
        free(stage_full_path);
    }
    free(exec_path);
    return success;
#endif
}

bool
run_script(const char *pkg_dir, const char *name, const char *root_path)
{
    char *scripts_dir = concat_dirs(pkg_dir, "scripts");
    if (!scripts_dir)
        return false;

    DIR *dir = opendir(scripts_dir);
    if (!dir)
    {
        free(scripts_dir);
        return true;
    }

    char name_norm[256];
    normalize(name, name_norm, sizeof(name_norm));

    char *found = NULL;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        char entry_norm[256];
        normalize(entry->d_name, entry_norm, sizeof(entry_norm));

        if (strcmp(entry_norm, name_norm) == 0)
        {
            found = concat_dirs(scripts_dir, entry->d_name);
            break;
        }
    }

    closedir(dir);
    free(scripts_dir);

    if (!found)
        return true;

    if (access(found, X_OK) != 0)
    {
        free(found);
        return true;
    }

    bool ok = exec_script(found, root_path);
    free(found);
    return ok;
}

static void
mkdir_p(const char *path)
{
    char buf[PATH_MAX];
    size_t len = strlen(path);
    if (len >= sizeof(buf))
        return;
    memcpy(buf, path, len + 1);

    for (size_t i = 1; i < len; i++)
    {
        if (buf[i] == '/')
        {
            buf[i] = '\0';
            create_dir(buf);
            buf[i] = '/';
        }
    }
    create_dir(buf);
}

static void
rmtree(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        unlink(path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
        char *child = concat_dirs(path, entry->d_name);
        if (child)
        {
            rmtree(child);
            free(child);
        }
    }
    closedir(dir);
    rmdir(path);
}

char *
scripts_store_path(const char *root_path, const char *pkg_name)
{
    char *base = concat_dirs(root_path, "var/lib/apg/pkgscripts");
    if (!base)
        return NULL;
    char *full = concat_dirs(base, pkg_name);
    free(base);
    return full;
}

bool
scripts_persist(const char *pkg_dir, const char *root_path,
                const char *pkg_name)
{
    char *src = concat_dirs(pkg_dir, "scripts");
    if (!src)
        return false;

    struct stat st;
    if (stat(src, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        free(src);
        return true;
    }

    char *store = scripts_store_path(root_path, pkg_name);
    if (!store)
    {
        free(src);
        return false;
    }

    mkdir_p(store);
    char *dst = concat_dirs(store, "scripts");
    bool ok = dst && copy_dir(src, dst);

    free(dst);
    free(store);
    free(src);
    return ok;
}

void
scripts_persist_remove(const char *root_path, const char *pkg_name)
{
    char *store = scripts_store_path(root_path, pkg_name);
    if (!store)
        return;
    rmtree(store);
    free(store);
}
