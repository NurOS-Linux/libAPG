// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/error.h>
#include <apg/package.h>
#include <apg/scripts.h>
#include <apg/transaction.h>

#include "fs_helpers.h"

#ifdef __linux__

#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sched.h>
#include <stddef.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>

static bool
isolation_available(void)
{
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0)
        _exit(unshare(CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWNS |
                      CLONE_NEWUTS | CLONE_NEWIPC) < 0
                  ? 1
                  : 0);
    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static void
deny_unshare(void)
{
    struct sock_filter filter[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_unshare, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    struct sock_fprog prog = {
        .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
        .filter = filter,
    };
    assert(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0);
    assert(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) == 0);
}

static void
run_in_child(void (*fn)(void))
{
    fflush(stdout);
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0)
    {
        fn();
        fflush(stdout);
        _exit(0);
    }
    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

static char *
make_script_dir(const char *body)
{
    char *dir = fs_mktmp("scripts");
    char *scripts = fs_join(dir, "scripts");
    fs_mkdir_p(scripts);
    char *script = fs_join(scripts, "post-install");
    fs_write(script, body);
    assert(chmod(script, 0755) == 0);
    free(script);
    free(scripts);
    return dir;
}

void
test_run_script_strict_matches_run_script_when_isolated(void)
{
    if (!isolation_available())
    {
        printf("test_run_script_strict_matches_run_script_when_isolated: SKIP "
               "(namespaces unavailable)\n");
        return;
    }

    char *ok_dir = make_script_dir("#!/bin/sh\nexit 0\n");
    char *fail_dir = make_script_dir("#!/bin/sh\nexit 3\n");

    assert(run_script_strict(ok_dir, "post-install", "/"));
    assert(run_script_strict(ok_dir, "post-install", NULL));
    assert(!run_script_strict(fail_dir, "post-install", "/"));
    assert(apg_last_error() && strstr(apg_last_error(), "status 3"));

    assert(run_script_strict(ok_dir, "pre-install", "/"));

    char *empty = fs_mktmp("scripts-none");
    assert(run_script_strict(empty, "post-install", "/"));

    char *script = fs_join(ok_dir, "scripts/post-install");
    assert(chmod(script, 0644) == 0);
    assert(run_script_strict(ok_dir, "post-install", "/"));
    free(script);

    fs_rmtree(ok_dir);
    fs_rmtree(fail_dir);
    fs_rmtree(empty);
    free(ok_dir);
    free(fail_dir);
    free(empty);
    printf("test_run_script_strict_matches_run_script_when_isolated: PASS\n");
}

static void
denied_script_case(void)
{
    char *dir = make_script_dir("#!/bin/sh\ntouch \"$0.ran\"\nexit 0\n");
    char *marker = fs_join(dir, "scripts/post-install.ran");

    deny_unshare();

    assert(run_script(dir, "post-install", "/"));
    assert(access(marker, F_OK) == 0);
    unlink(marker);

    assert(!run_script_strict(dir, "post-install", "/"));
    assert(access(marker, F_OK) != 0);
    assert(apg_last_error() && strstr(apg_last_error(), "isolate"));

    assert(run_script_strict(dir, "pre-install", "/"));

    char *empty = fs_mktmp("scripts-none");
    assert(run_script_strict(empty, "post-install", "/"));

    fs_rmtree(dir);
    fs_rmtree(empty);
    free(marker);
    free(dir);
    free(empty);
}

void
test_run_script_strict_refuses_when_isolation_is_denied(void)
{
    run_in_child(denied_script_case);
    printf("test_run_script_strict_refuses_when_isolation_is_denied: PASS\n");
}

static char *
build_script_package(const char *name)
{
    char *src = fs_mktmp("sandbox-src");
    char *data = fs_join(src, "data/usr/share");
    char *scripts = fs_join(src, "scripts");
    fs_mkdir_p(data);
    fs_mkdir_p(scripts);

    char *meta = fs_join(src, "metadata.json");
    char json[256];
    snprintf(json, sizeof(json),
             "{\"name\": \"%s\", \"version\": \"1.0.0\", \"type\": \"app\"}",
             name);
    fs_write(meta, json);

    char *file = fs_join(data, "payload");
    fs_write(file, "payload");

    char *script = fs_join(scripts, "post-install");
    fs_write(script, "#!/bin/sh\ntouch /script-ran\nexit 0\n");
    assert(chmod(script, 0755) == 0);

    char *archive = fs_join(src, "pkg.tar.gz");
    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd),
             "tar -czf '%s' -C '%s' metadata.json data scripts", archive, src);
    assert(system(cmd) == 0);

    free(data);
    free(scripts);
    free(meta);
    free(file);
    free(script);
    free(src);
    return archive;
}

static void
remove_package_source(char *archive)
{
    char *slash = strrchr(archive, '/');
    *slash = '\0';
    fs_rmtree(archive);
    *slash = '/';
    free(archive);
}

static void
commit_case(bool require_sandbox, bool expect_isolation_message)
{
    char db_path[PATH_MAX];
    struct db_handle *db = fs_open_db(db_path);
    assert(db);
    char *root = fs_mktmp("sandbox-root");
    char *archive = build_script_package("sandboxed");

    struct package *pkg = parse_package(archive, root);
    assert(pkg);

    deny_unshare();

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    trans_set_require_sandbox(trans, require_sandbox);
    assert(trans_prepare(trans) == TRANS_OK);

    assert(trans_commit(trans, root) == TRANS_ERR_INSTALL_FAILED);
    const char *message = apg_last_error();
    assert(message && strstr(message, "post-install"));
    assert((strstr(message, "isolate") != NULL) == expect_isolation_message);

    char *marker = fs_join(root, "script-ran");
    assert(access(marker, F_OK) != 0);
    struct package *recorded = db_get(db, "sandboxed");
    assert(recorded == NULL);

    free(marker);
    trans_free(trans);
    package_free(pkg);
    remove_package_source(archive);
    fs_rmtree(root);
    free(root);
    fs_close_db(db, db_path);
}

static void
commit_denied_default_case(void)
{
    commit_case(false, false);
}

static void
commit_denied_required_case(void)
{
    commit_case(true, true);
}

void
test_trans_require_sandbox_controls_script_isolation(void)
{
    trans_set_require_sandbox(NULL, true);
    run_in_child(commit_denied_default_case);
    run_in_child(commit_denied_required_case);
    printf("test_trans_require_sandbox_controls_script_isolation: PASS\n");
}

#else

void
test_run_script_strict_matches_run_script_when_isolated(void)
{
    printf("test_run_script_strict_matches_run_script_when_isolated: SKIP "
           "(not Linux)\n");
}

void
test_run_script_strict_refuses_when_isolation_is_denied(void)
{
    printf("test_run_script_strict_refuses_when_isolation_is_denied: SKIP "
           "(not Linux)\n");
}

void
test_trans_require_sandbox_controls_script_isolation(void)
{
    trans_set_require_sandbox(NULL, true);
    printf("test_trans_require_sandbox_controls_script_isolation: SKIP "
           "(not Linux)\n");
}

#endif
