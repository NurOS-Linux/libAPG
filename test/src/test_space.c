// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/error.h>
#include <apg/package.h>
#include <apg/transaction.h>

#include "fs_helpers.h"

#define BLOCK 4096

static void
write_sized(const char *path, size_t size)
{
    FILE *f = fopen(path, "wb");
    assert(f);
    for (size_t i = 0; i < size; i++)
        fputc('x', f);
    fclose(f);
}

static char *
build_archive(const char *name, size_t big, size_t small)
{
    char *src = fs_mktmp("space-src");
    char *data = fs_join(src, "data/usr/share");
    char *scripts = fs_join(src, "scripts");
    fs_mkdir_p(data);
    fs_mkdir_p(scripts);

    char *meta = fs_join(src, "metadata.json");
    char json[256];
    snprintf(json, sizeof(json),
             "{\"name\": \"%s\", \"version\": \"2.0.0\", \"type\": \"app\"}",
             name);
    fs_write(meta, json);

    char *big_file = fs_join(data, "big.bin");
    char *small_file = fs_join(data, "small.bin");
    write_sized(big_file, big);
    write_sized(small_file, small);

    char *script = fs_join(scripts, "post-install");
    fs_write(script, "#!/bin/sh\nexit 0\n");
    assert(chmod(script, 0755) == 0);

    char *archive = fs_join(src, "pkg.tar.gz");
    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd),
             "tar -czf '%s' -C '%s' metadata.json data scripts", archive, src);
    assert(system(cmd) == 0);

    free(data);
    free(scripts);
    free(meta);
    free(big_file);
    free(small_file);
    free(script);
    free(src);
    return archive;
}

static void
remove_archive(char *archive)
{
    char *slash = strrchr(archive, '/');
    *slash = '\0';
    fs_rmtree(archive);
    free(archive);
}

static uint64_t
blocks(size_t size)
{
    return (size + BLOCK - 1) / BLOCK * BLOCK;
}

static struct package *
load_package(const char *archive, const char *root, const char *file)
{
    struct package *pkg = parse_package(archive, root);
    assert(pkg);
    pkg->package_files.items = calloc(1, sizeof(char *));
    pkg->package_files.items[0] = strdup(file);
    pkg->package_files.count = 1;
    return pkg;
}

void
test_trans_estimate_space_for_installs(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = fs_open_db(db_path);
    assert(db);
    char *root = fs_mktmp("space-root");

    char *archive_a = build_archive("pkg-a", 10000, 1);
    char *archive_b = build_archive("pkg-b", 100000, 5000);
    struct package *a = load_package(archive_a, root, "/usr/share/a.bin");
    struct package *b = load_package(archive_b, root, "/usr/share/b.bin");

    uint64_t meta_blocks = BLOCK;
    uint64_t script_blocks = BLOCK;
    uint64_t installed_a = blocks(10000) + blocks(1) + script_blocks;
    uint64_t installed_b = blocks(100000) + blocks(5000) + script_blocks;
    uint64_t extracted_a = installed_a + meta_blocks;
    uint64_t extracted_b = installed_b + meta_blocks;

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, a) == TRANS_OK);

    uint64_t needed = 0;
    uint64_t available = 0;
    assert(!trans_estimate_space(trans, root, &needed, &available));
    assert(apg_last_error() && strstr(apg_last_error(), "not prepared"));

    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_estimate_space(trans, root, &needed, &available));
    assert(needed == installed_a + extracted_a);

    struct statvfs vfs;
    assert(statvfs(root, &vfs) == 0);
    assert(available > 0);
    assert(available <= (uint64_t)vfs.f_blocks * vfs.f_frsize);
    trans_free(trans);

    trans = trans_new(db);
    assert(trans_add_install(trans, a) == TRANS_OK);
    assert(trans_add_install(trans, b) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_estimate_space(trans, root, &needed, &available));
    assert(needed == installed_a + installed_b + extracted_b);

    char *missing_root = fs_join(root, "not/yet/created");
    assert(trans_estimate_space(trans, missing_root, &needed, &available));
    assert(needed == installed_a + installed_b + extracted_b);
    free(missing_root);

    assert(!trans_estimate_space(NULL, root, &needed, &available));
    assert(!trans_estimate_space(trans, NULL, &needed, &available));
    assert(!trans_estimate_space(trans, root, NULL, &available));
    assert(!trans_estimate_space(trans, root, &needed, NULL));

    trans_free(trans);
    package_free(a);
    package_free(b);
    remove_archive(archive_a);
    remove_archive(archive_b);
    fs_rmtree(root);
    free(root);
    fs_close_db(db, db_path);
    printf("test_trans_estimate_space_for_installs: PASS\n");
}

void
test_trans_estimate_space_counts_upgrade_backups(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = fs_open_db(db_path);
    assert(db);
    char *root = fs_mktmp("space-upgrade-root");

    char *share = fs_join(root, "usr/share");
    fs_mkdir_p(share);
    char *old_big = fs_join(share, "old-big");
    char *old_small = fs_join(share, "old-small");
    write_sized(old_big, 5000);
    write_sized(old_small, 1);

    char *store = fs_join(root, "var/lib/apg/pkgscripts/up/scripts");
    fs_mkdir_p(store);
    char *stored = fs_join(store, "post-install");
    fs_write(stored, "#!/bin/sh\nexit 0\n");

    const char *old_files[] = {"/usr/share/old-big", "/usr/share/old-small"};
    struct package *old = fs_make_package("up", "1.0.0", NULL, NULL, 0, NULL, 0,
                                          old_files, 2, true);
    assert(db_add(db, old));
    package_free(old);

    char *archive = build_archive("up", 10000, 1);
    struct package *fresh = load_package(archive, root, "/usr/share/new.bin");

    uint64_t installed = blocks(10000) + blocks(1) + BLOCK;
    uint64_t extracted = installed + BLOCK;
    uint64_t backup = blocks(5000) + blocks(1) + BLOCK;

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_upgrade(trans, fresh) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    uint64_t needed = 0;
    uint64_t available = 0;
    assert(trans_estimate_space(trans, root, &needed, &available));
    assert(needed == installed + extracted + backup);

    trans_free(trans);
    package_free(fresh);
    remove_archive(archive);
    free(share);
    free(old_big);
    free(old_small);
    free(store);
    free(stored);
    fs_rmtree(root);
    free(root);
    fs_close_db(db, db_path);
    printf("test_trans_estimate_space_counts_upgrade_backups: PASS\n");
}

void
test_trans_estimate_space_edge_cases(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = fs_open_db(db_path);
    assert(db);
    char *root = fs_mktmp("space-edge-root");

    struct apg_trans *trans = trans_new(db);
    assert(trans_prepare(trans) == TRANS_OK);
    uint64_t needed = 99;
    uint64_t available = 0;
    assert(trans_estimate_space(trans, root, &needed, &available));
    assert(needed == 0 && available > 0);
    trans_free(trans);

    const char *files[] = {"/usr/bin/gone"};
    struct package *installed =
        fs_make_package("gone", "1.0", NULL, NULL, 0, NULL, 0, files, 1, true);
    assert(db_add(db, installed));
    package_free(installed);
    trans = trans_new(db);
    assert(trans_add_remove(trans, "gone") == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_estimate_space(trans, root, &needed, &available));
    assert(needed == 0);
    trans_free(trans);

    const char *noarch_files[] = {"/usr/bin/noarch"};
    struct package *no_archive = fs_make_package(
        "noarch", "1.0", NULL, NULL, 0, NULL, 0, noarch_files, 1, true);
    free(no_archive->pkg_path);
    no_archive->pkg_path = NULL;
    trans = trans_new(db);
    assert(trans_add_install(trans, no_archive) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);
    assert(!trans_estimate_space(trans, root, &needed, &available));
    assert(apg_last_error() && strstr(apg_last_error(), "noarch"));
    trans_free(trans);
    package_free(no_archive);

    const char *badarch_files[] = {"/usr/bin/badarch"};
    struct package *bad_archive = fs_make_package(
        "badarch", "1.0", NULL, NULL, 0, NULL, 0, badarch_files, 1, true);
    bad_archive->pkg_path = strdup("/nonexistent/archive.apg");
    trans = trans_new(db);
    assert(trans_add_install(trans, bad_archive) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);
    assert(!trans_estimate_space(trans, root, &needed, &available));
    assert(apg_last_error() && strstr(apg_last_error(), "archive"));
    trans_free(trans);
    package_free(bad_archive);

    fs_rmtree(root);
    free(root);
    fs_close_db(db, db_path);
    printf("test_trans_estimate_space_edge_cases: PASS\n");
}
