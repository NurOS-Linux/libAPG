// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <lmdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/journal.h>
#include <apg/package.h>
#include <apg/transaction.h>

#include "fs_helpers.h"

static MDB_env *
open_env(const char *path)
{
    MDB_env *env = NULL;
    assert(mdb_env_create(&env) == MDB_SUCCESS);
    assert(mdb_env_set_maxdbs(env, 4) == MDB_SUCCESS);
    assert(mdb_env_set_mapsize(env, 4 * 1024 * 1024) == MDB_SUCCESS);
    assert(mdb_env_open(env, path, 0, 0644) == MDB_SUCCESS);
    return env;
}

void
test_journal_read_without_journal_is_empty(void)
{
    char *dir = fs_mktmp("jr-empty");
    MDB_env *env = open_env(dir);

    int count = -1;
    struct journal_entry **list = journal_read_all(env, &count);
    assert(list == NULL && count == 0);

    mdb_env_close(env);
    fs_rmtree(dir);
    free(dir);
    printf("test_journal_read_without_journal_is_empty: PASS\n");
}

void
test_journal_write_read_roundtrip(void)
{
    char *dir = fs_mktmp("jr-rt");
    MDB_env *env = open_env(dir);

    time_t before = time(NULL);
    assert(journal_write(env, JOURNAL_INSTALL, "alpha", "1.2.3",
                         JOURNAL_STATUS_OK, 1000, true));
    assert(journal_write(env, JOURNAL_REMOVE, "beta", "2:4.0-1",
                         JOURNAL_STATUS_FAILED, 0, false));
    assert(journal_write(env, JOURNAL_ROLLBACK, "gamma", "0.1",
                         JOURNAL_STATUS_OK, 4000000000u, false));
    time_t after = time(NULL);

    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    assert(list && count == 3);

    assert(journal_entry_op(list[0]) == JOURNAL_INSTALL);
    assert(strcmp(journal_entry_pkg_name(list[0]), "alpha") == 0);
    assert(strcmp(journal_entry_pkg_version(list[0]), "1.2.3") == 0);
    assert(journal_entry_status(list[0]) == JOURNAL_STATUS_OK);
    assert(journal_entry_uid(list[0]) == 1000);
    assert(journal_entry_explicit(list[0]));

    assert(journal_entry_op(list[1]) == JOURNAL_REMOVE);
    assert(strcmp(journal_entry_pkg_version(list[1]), "2:4.0-1") == 0);
    assert(journal_entry_status(list[1]) == JOURNAL_STATUS_FAILED);
    assert(journal_entry_uid(list[1]) == 0);
    assert(!journal_entry_explicit(list[1]));

    assert(journal_entry_op(list[2]) == JOURNAL_ROLLBACK);
    assert(journal_entry_uid(list[2]) == 4000000000u);

    for (int i = 0; i < count; i++)
    {
        time_t ts = journal_entry_timestamp(list[i]);
        assert(ts >= before && ts <= after);
    }

    journal_free_all(list, count);
    journal_entry_free(NULL);
    journal_free_all(NULL, 0);
    mdb_env_close(env);
    fs_rmtree(dir);
    free(dir);
    printf("test_journal_write_read_roundtrip: PASS\n");
}

void
test_journal_preserves_write_order(void)
{
    char *dir = fs_mktmp("jr-order");
    MDB_env *env = open_env(dir);

    for (int i = 0; i < 100; i++)
    {
        char ver[16];
        snprintf(ver, sizeof(ver), "v%d", i);
        assert(journal_write(env, JOURNAL_INSTALL, "same", ver,
                             JOURNAL_STATUS_OK, 0, true));
    }

    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    assert(list && count == 100);
    for (int i = 0; i < count; i++)
    {
        char ver[16];
        snprintf(ver, sizeof(ver), "v%d", i);
        assert(strcmp(journal_entry_pkg_version(list[i]), ver) == 0);
    }

    journal_free_all(list, count);
    mdb_env_close(env);
    fs_rmtree(dir);
    free(dir);
    printf("test_journal_preserves_write_order: PASS\n");
}

void
test_journal_null_name_and_version_roundtrip(void)
{
    char *dir = fs_mktmp("jr-null");
    MDB_env *env = open_env(dir);

    assert(journal_write(env, JOURNAL_INSTALL, NULL, NULL, JOURNAL_STATUS_OK, 7,
                         true));
    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    assert(list && count == 1);
    assert(journal_entry_pkg_name(list[0]) == NULL);
    assert(journal_entry_pkg_version(list[0]) == NULL);
    assert(journal_entry_uid(list[0]) == 7);
    assert(journal_entry_explicit(list[0]));

    journal_free_all(list, count);
    mdb_env_close(env);
    fs_rmtree(dir);
    free(dir);
    printf("test_journal_null_name_and_version_roundtrip: PASS\n");
}

void
test_journal_fields_with_separator_survive(void)
{
    char *dir = fs_mktmp("jr-sep");
    MDB_env *env = open_env(dir);

    assert(journal_write(env, JOURNAL_INSTALL, "we|ird", "1|2",
                         JOURNAL_STATUS_FAILED, 42, true));
    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    assert(list && count == 1);
    assert(strcmp(journal_entry_pkg_name(list[0]), "we|ird") == 0);
    assert(strcmp(journal_entry_pkg_version(list[0]), "1|2") == 0);
    assert(journal_entry_status(list[0]) == JOURNAL_STATUS_FAILED);
    assert(journal_entry_uid(list[0]) == 42);
    assert(journal_entry_explicit(list[0]));

    journal_free_all(list, count);
    mdb_env_close(env);
    fs_rmtree(dir);
    free(dir);
    printf("test_journal_fields_with_separator_survive: PASS\n");
}

void
test_journal_long_names_are_not_silently_corrupted(void)
{
    char *dir = fs_mktmp("jr-long");
    MDB_env *env = open_env(dir);

    char name[1025];
    memset(name, 'n', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    bool written = journal_write(env, JOURNAL_REMOVE, name, "1.0",
                                 JOURNAL_STATUS_FAILED, 99, true);
    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    if (written)
    {
        assert(list && count == 1);
        assert(strcmp(journal_entry_pkg_name(list[0]), name) == 0);
        assert(strcmp(journal_entry_pkg_version(list[0]), "1.0") == 0);
        assert(journal_entry_status(list[0]) == JOURNAL_STATUS_FAILED);
        assert(journal_entry_uid(list[0]) == 99);
    }
    else
    {
        assert(count == 0);
    }

    journal_free_all(list, count);
    mdb_env_close(env);
    fs_rmtree(dir);
    free(dir);
    printf("test_journal_long_names_are_not_silently_corrupted: PASS\n");
}

void
test_journal_db_operations_are_recorded(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    struct package *manual =
        fs_make_package("manual", "1.0", NULL, NULL, 0, NULL, 0, NULL, 0, true);
    struct package *dep =
        fs_make_package("dep", "2.0", NULL, NULL, 0, NULL, 0, NULL, 0, false);
    assert(db_add(db, manual));
    assert(db_add(db, dep));
    assert(db_set_hold(db, "manual", true));
    assert(db_set_installed_by_hand(db, "dep", true));
    assert(db_remove(db, "dep"));
    package_free(manual);
    package_free(dep);
    db_close(db);

    MDB_env *env = open_env(path);
    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    assert(list && count == 3);

    assert(journal_entry_op(list[0]) == JOURNAL_INSTALL);
    assert(strcmp(journal_entry_pkg_name(list[0]), "manual") == 0);
    assert(strcmp(journal_entry_pkg_version(list[0]), "1.0") == 0);
    assert(journal_entry_status(list[0]) == JOURNAL_STATUS_OK);
    assert(journal_entry_explicit(list[0]));
    assert(journal_entry_uid(list[0]) == getuid());

    assert(journal_entry_op(list[1]) == JOURNAL_INSTALL);
    assert(strcmp(journal_entry_pkg_name(list[1]), "dep") == 0);

    assert(journal_entry_op(list[2]) == JOURNAL_REMOVE);
    assert(strcmp(journal_entry_pkg_name(list[2]), "dep") == 0);
    assert(journal_entry_status(list[2]) == JOURNAL_STATUS_OK);

    journal_free_all(list, count);
    mdb_env_close(env);
    fs_rmtree(path);
    printf("test_journal_db_operations_are_recorded: PASS\n");
}

void
test_journal_removing_missing_package_is_not_recorded_as_ok(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    bool removed = db_remove(db, "ghost");
    db_close(db);

    MDB_env *env = open_env(path);
    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    for (int i = 0; i < count; i++)
    {
        if (journal_entry_pkg_name(list[i]) &&
            strcmp(journal_entry_pkg_name(list[i]), "ghost") == 0)
            assert(removed ||
                   journal_entry_status(list[i]) == JOURNAL_STATUS_FAILED);
    }

    journal_free_all(list, count);
    mdb_env_close(env);
    fs_rmtree(path);
    printf("test_journal_removing_missing_package_is_not_recorded_as_ok: "
           "PASS\n");
}

static size_t
last_txnid(const char *path)
{
    MDB_env *env = NULL;
    assert(mdb_env_create(&env) == MDB_SUCCESS);
    assert(mdb_env_set_maxdbs(env, 4) == MDB_SUCCESS);
    assert(mdb_env_open(env, path, MDB_RDONLY, 0644) == MDB_SUCCESS);
    MDB_envinfo info;
    assert(mdb_env_info(env, &info) == MDB_SUCCESS);
    mdb_env_close(env);
    return info.me_last_txnid;
}

void
test_db_record_and_journal_share_one_commit(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);
    db_close(db);
    size_t base = last_txnid(path);

    db = db_open(path);
    db_close(db);
    assert(last_txnid(path) == base);

    db = db_open(path);
    struct package *pkg = fs_make_package("one-commit", "1.0", NULL, NULL, 0,
                                          NULL, 0, NULL, 0, true);
    assert(db_add(db, pkg));
    db_close(db);
    size_t after_add = last_txnid(path);
    assert(after_add == base + 1);

    db = db_open(path);
    assert(db_set_hold(db, "one-commit", true));
    db_close(db);
    size_t after_hold = last_txnid(path);
    assert(after_hold == after_add + 1);

    db = db_open(path);
    assert(db_remove(db, "one-commit"));
    db_close(db);
    size_t after_remove = last_txnid(path);
    assert(after_remove == after_hold + 1);

    db = db_open(path);
    assert(!db_remove(db, "one-commit"));
    db_close(db);
    assert(last_txnid(path) == after_remove + 1);

    package_free(pkg);

    MDB_env *env = open_env(path);
    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    assert(list && count == 3);
    assert(journal_entry_op(list[0]) == JOURNAL_INSTALL);
    assert(journal_entry_status(list[0]) == JOURNAL_STATUS_OK);
    assert(strcmp(journal_entry_pkg_version(list[0]), "1.0") == 0);
    assert(journal_entry_op(list[1]) == JOURNAL_REMOVE);
    assert(journal_entry_status(list[1]) == JOURNAL_STATUS_OK);
    assert(strcmp(journal_entry_pkg_version(list[1]), "1.0") == 0);
    assert(journal_entry_op(list[2]) == JOURNAL_REMOVE);
    assert(journal_entry_status(list[2]) == JOURNAL_STATUS_FAILED);
    journal_free_all(list, count);
    mdb_env_close(env);

    fs_rmtree(path);
    printf("test_db_record_and_journal_share_one_commit: PASS\n");
}

static char *
build_plain_archive(const char *name)
{
    char *src = fs_mktmp("commit-count-src");
    char *data = fs_join(src, "data/usr/share");
    fs_mkdir_p(data);
    char *meta = fs_join(src, "metadata.json");
    char json[256];
    snprintf(json, sizeof(json),
             "{\"name\": \"%s\", \"version\": \"1.0.0\", \"type\": \"app\"}",
             name);
    fs_write(meta, json);
    char *file = fs_join(data, "payload");
    fs_write(file, "payload");

    char *archive = fs_join(src, "pkg.tar.gz");
    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd), "tar -czf '%s' -C '%s' metadata.json data",
             archive, src);
    assert(system(cmd) == 0);

    free(data);
    free(meta);
    free(file);
    free(src);
    return archive;
}

void
test_trans_commit_uses_one_commit_per_step(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);
    char *root = fs_mktmp("commit-count-root");
    char *archive = build_plain_archive("counted");

    for (int i = 0; i < 3; i++)
    {
        char name[16];
        snprintf(name, sizeof(name), "old%d", i);
        struct package *pkg =
            fs_make_package(name, "1.0", NULL, NULL, 0, NULL, 0, NULL, 0, true);
        assert(db_add(db, pkg));
        package_free(pkg);
    }
    db_close(db);
    size_t base = last_txnid(path);

    db = db_open(path);
    struct apg_trans *trans = trans_new(db);
    for (int i = 0; i < 3; i++)
    {
        char name[16];
        snprintf(name, sizeof(name), "old%d", i);
        assert(trans_add_remove(trans, name) == TRANS_OK);
    }
    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_commit(trans, root) == TRANS_OK);
    trans_free(trans);
    db_close(db);
    size_t after_removals = last_txnid(path);
    assert(after_removals == base + 3);

    db = db_open(path);
    struct package *pkg = parse_package(archive, root);
    assert(pkg);
    trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_commit(trans, root) == TRANS_OK);
    trans_free(trans);
    package_free(pkg);
    db_close(db);
    assert(last_txnid(path) == after_removals + 1);

    MDB_env *env = open_env(path);
    int count = 0;
    struct journal_entry **list = journal_read_all(env, &count);
    assert(list && count == 3 + 3 + 1);
    int removals_ok = 0;
    int installs_ok = 0;
    for (int i = 0; i < count; i++)
    {
        if (journal_entry_status(list[i]) != JOURNAL_STATUS_OK)
            continue;
        if (journal_entry_op(list[i]) == JOURNAL_REMOVE &&
            journal_entry_explicit(list[i]))
            removals_ok++;
        if (journal_entry_op(list[i]) == JOURNAL_INSTALL &&
            strcmp(journal_entry_pkg_name(list[i]), "counted") == 0 &&
            journal_entry_explicit(list[i]))
            installs_ok++;
    }
    assert(removals_ok == 3 && installs_ok == 1);
    journal_free_all(list, count);
    mdb_env_close(env);

    char *slash = strrchr(archive, '/');
    *slash = '\0';
    fs_rmtree(archive);
    free(archive);
    fs_rmtree(root);
    free(root);
    fs_rmtree(path);
    printf("test_trans_commit_uses_one_commit_per_step: PASS\n");
}
