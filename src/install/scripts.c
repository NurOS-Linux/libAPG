// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __linux__
#include <sched.h>
#include <sys/mount.h>
#elif defined(__FreeBSD__)
#include <fcntl.h>
#include <sys/capsicum.h>
#endif

#include "../../include/apg/copy.h"
#include "../../include/apg/scripts.h"
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

static bool
exec_script(const char *path, const char *root_path)
{
    bool do_chroot = (root_path != NULL && strcmp(root_path, "/") != 0 &&
                      *root_path != '\0');
    char *exec_path = NULL;
    bool is_temp_script = false;

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
            char stage_dir[PATH_MAX];
            snprintf(stage_dir, sizeof(stage_dir), "%.*s/tmp", (int)root_len,
                     root_path);
            create_dir(stage_dir);

            char stage_path[PATH_MAX];
            snprintf(stage_path, sizeof(stage_path), "%.*s/tmp/.apg_script_tmp",
                     (int)root_len, root_path);

            bool cpy_ok = copy_file(path, stage_path);
            if (cpy_ok)
                chmod(stage_path, 0755);

            if (!cpy_ok)
                return false;

            is_temp_script = true;
            exec_path = strdup("/tmp/.apg_script_tmp");
        }

        if (!exec_path)
            return false;
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
        if (is_temp_script && do_chroot)
        {
            char stage[PATH_MAX];
            snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
            unlink(stage);
        }
        free(exec_path);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        if (is_temp_script && do_chroot)
        {
            char stage[PATH_MAX];
            snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
            unlink(stage);
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
            char check_sh[PATH_MAX];
            snprintf(check_sh, sizeof(check_sh), "%s/bin/sh", root_path);
            if (access(check_sh, F_OK) != 0)
            {
                const char *bind_dirs[] = {"/bin", "/lib", "/lib64", "/usr"};
                for (size_t b = 0; b < sizeof(bind_dirs) / sizeof(bind_dirs[0]);
                     b++)
                {
                    if (access(bind_dirs[b], F_OK) == 0)
                    {
                        char target_dir[PATH_MAX];
                        snprintf(target_dir, sizeof(target_dir), "%s%s",
                                 root_path, bind_dirs[b]);
                        create_dir(target_dir);
                        mount(bind_dirs[b], target_dir, NULL, MS_BIND | MS_REC,
                              NULL);
                    }
                }
            }

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

    if (is_temp_script && do_chroot)
    {
        char stage[PATH_MAX];
        snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
        unlink(stage);
    }
    free(exec_path);
    return success;

#elif defined(__FreeBSD__)
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        if (is_temp_script && do_chroot)
        {
            char stage[PATH_MAX];
            snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
            unlink(stage);
        }
        free(exec_path);
        return false;
    }

    int fd = open(do_chroot ? path : exec_path, O_EXEC);
    if (fd < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        if (is_temp_script && do_chroot)
        {
            char stage[PATH_MAX];
            snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
            unlink(stage);
        }
        free(exec_path);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(fd);
        close(pipefd[0]);
        close(pipefd[1]);
        if (is_temp_script && do_chroot)
        {
            char stage[PATH_MAX];
            snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
            unlink(stage);
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

        if (cap_enter() < 0)
        {
            uint8_t err = 1;
            (void)write(pipefd[1], &err, 1);
            close(pipefd[1]);
            _exit(1);
        }

        close(pipefd[1]);
        char *const argv[] = {(char *)exec_path, NULL};
        char *const envp[] = {NULL};
        fexecve(fd, argv, envp);
        _exit(1);
    }

    close(fd);
    close(pipefd[1]);
    uint8_t err = 0;
    ssize_t n = read(pipefd[0], &err, 1);
    close(pipefd[0]);

    if (is_temp_script && do_chroot)
    {
        char stage[PATH_MAX];
        snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
        unlink(stage);
    }
    free(exec_path);

    if (n > 0)
    {
        waitpid(pid, NULL, 0);
        return false;
    }

    int status;
    if (waitpid(pid, &status, 0) < 0)
        return false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#else
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        if (is_temp_script && do_chroot)
        {
            char stage[PATH_MAX];
            snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
            unlink(stage);
        }
        free(exec_path);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        if (is_temp_script && do_chroot)
        {
            char stage[PATH_MAX];
            snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
            unlink(stage);
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

    if (is_temp_script && do_chroot)
    {
        char stage[PATH_MAX];
        snprintf(stage, sizeof(stage), "%s/tmp/.apg_script_tmp", root_path);
        unlink(stage);
    }
    free(exec_path);

    if (n > 0)
    {
        waitpid(pid, NULL, 0);
        return false;
    }

    int status;
    if (waitpid(pid, &status, 0) < 0)
        return false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
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
