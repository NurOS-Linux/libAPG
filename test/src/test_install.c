// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <apg/install.h>
#include <apg/db.h>
#include <apg/package.h>
#include <apg/scripts.h>
#include <util.h>

// concat_dirs() does not insert a separator between its arguments (despite
// its docstring), so path construction in this file uses plain snprintf.
static char *
join_path(const char *a, const char *b)
{
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s/%s", a, b);
    return strdup(buf);
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
        char *child = join_path(path, entry->d_name);
        if (child)
        {
            rmtree(child);
            free(child);
        }
    }
    closedir(dir);
    rmdir(path);
}

static void
mkdir_p(const char *path)
{
    char buf[PATH_MAX];
    size_t len = strlen(path);
    assert(len < sizeof(buf));
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
write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "wb");
    assert(f);
    fwrite(content, 1, strlen(content), f);
    fclose(f);
}

static bool
file_contains(const char *path, const char *expected)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    char buf[256] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    return n == strlen(expected) && memcmp(buf, expected, n) == 0;
}

static char *
mktmp_dir(const char *suffix)
{
    char tmpl[PATH_MAX];
    snprintf(tmpl, sizeof(tmpl), "/tmp/apg-test-%s-XXXXXX", suffix);
    char *made = mkdtemp(tmpl);
    assert(made);
    return strdup(made);
}

void
test_install_data_dir_copies_files(void)
{
    char *pkg_dir = mktmp_dir("pkg");
    char *root = mktmp_dir("root");

    char *data_dir = join_path(pkg_dir, "data");
    char *nested_dir = join_path(data_dir, "etc/apg");
    mkdir_p(nested_dir);

    char *file_path = join_path(nested_dir, "config.conf");
    write_file(file_path, "hello from the package");

    assert(install_data_dir(pkg_dir, root));

    char *installed_path = join_path(root, "etc/apg/config.conf");
    struct stat st;
    assert(stat(installed_path, &st) == 0);
    assert(S_ISREG(st.st_mode));
    assert(file_contains(installed_path, "hello from the package"));

    free(file_path);
    free(nested_dir);
    free(data_dir);
    free(installed_path);
    rmtree(pkg_dir);
    rmtree(root);
    free(pkg_dir);
    free(root);
    printf("test_install_data_dir_copies_files: PASS\n");
}

void
test_install_data_dir_missing_data_returns_false(void)
{
    char *pkg_dir = mktmp_dir("nodata");
    char *root = mktmp_dir("root2");

    assert(!install_data_dir(pkg_dir, root));

    rmtree(pkg_dir);
    rmtree(root);
    free(pkg_dir);
    free(root);
    printf("test_install_data_dir_missing_data_returns_false: PASS\n");
}

void
test_rollback_install_removes_files(void)
{
    char *pkg_dir = mktmp_dir("pkg3");
    char *root = mktmp_dir("root3");

    char *data_dir = join_path(pkg_dir, "data");
    char *nested_dir = join_path(data_dir, "usr/bin");
    mkdir_p(nested_dir);

    char *file_path = join_path(nested_dir, "apg-tool");
    write_file(file_path, "#!/bin/sh\necho hi\n");

    assert(install_data_dir(pkg_dir, root));

    char *installed_path = join_path(root, "usr/bin/apg-tool");
    struct stat st;
    assert(stat(installed_path, &st) == 0);

    rollback_install(pkg_dir, root);

    assert(stat(installed_path, &st) != 0);
    char *installed_dir = join_path(root, "usr/bin");
    assert(stat(installed_dir, &st) != 0);

    free(file_path);
    free(nested_dir);
    free(data_dir);
    free(installed_path);
    free(installed_dir);
    rmtree(pkg_dir);
    rmtree(root);
    free(pkg_dir);
    free(root);
    printf("test_rollback_install_removes_files: PASS\n");
}

static struct db_handle *
open_tmp_db(char *buf)
{
    strcpy(buf, "/tmp/apg-test-db-XXXXXX");
    if (!mkdtemp(buf))
        return NULL;
    return db_open(buf);
}

static void
close_tmp_db(struct db_handle *db, const char *path)
{
    char f[PATH_MAX];
    db_close(db);
    snprintf(f, sizeof(f), "%s/data.mdb", path);
    unlink(f);
    snprintf(f, sizeof(f), "%s/lock.mdb", path);
    unlink(f);
    rmdir(path);
}

void
test_db_add_get_remove_roundtrip(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta->name = strdup("demo-pkg");
    pkg->meta->version = strdup("2.1.0");
    pkg->meta->description = strdup("A demo package for install tests");
    pkg->installed_by_hand = true;

    assert(db_add(db, pkg));

    struct package *fetched = db_get(db, "demo-pkg");
    assert(fetched);
    assert(strcmp(fetched->meta->name, "demo-pkg") == 0);
    assert(strcmp(fetched->meta->version, "2.1.0") == 0);
    assert(fetched->installed_by_hand);
    package_free(fetched);

    assert(db_remove(db, "demo-pkg"));
    assert(db_get(db, "demo-pkg") == NULL);

    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_db_add_get_remove_roundtrip: PASS\n");
}

void
test_run_script_root(void)
{
    char *pkg_dir = mktmp_dir("scriptpkg");
    char *scripts_dir = join_path(pkg_dir, "scripts");
    mkdir_p(scripts_dir);

    char *script_path = join_path(scripts_dir, "post-install");
    write_file(script_path, "#!/bin/sh\nexit 0\n");
    chmod(script_path, 0755);

    assert(run_script(pkg_dir, "post-install", "/"));
    assert(run_script(pkg_dir, "post-install", NULL));

    char *root = mktmp_dir("root_script");
    chmod(root, 0755);
    char *root_tmp = join_path(root, "tmp");
    mkdir_p(root_tmp);
    chmod(root_tmp, 0755);

    char *alt_pkg_dir = join_path(root, "tmp/pkg");
    char *alt_scripts_dir = join_path(alt_pkg_dir, "scripts");
    mkdir_p(alt_scripts_dir);

    char *marker_script = join_path(alt_scripts_dir, "pre-install");
    write_file(marker_script, "#!/bin/sh\necho hello > /marker_test.txt\n");
    chmod(marker_script, 0755);

    bool res = run_script(alt_pkg_dir, "pre-install", root);
    assert(res == true);

    char *marker_file = join_path(root, "marker_test.txt");
    struct stat st;
    assert(stat(marker_file, &st) == 0);

    free(marker_file);
    free(marker_script);
    free(alt_scripts_dir);
    free(alt_pkg_dir);
    free(root_tmp);
    free(script_path);
    free(scripts_dir);
    rmtree(pkg_dir);
    rmtree(root);
    free(pkg_dir);
    free(root);
    printf("test_run_script_root: PASS\n");
}
