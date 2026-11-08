// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <lmdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/db.h>
#include <apg/json.h>
#include <apg/package.h>

#include "fs_helpers.h"

struct raw_db
{
    MDB_env *env;
    MDB_txn *txn;
    MDB_dbi main;
    MDB_dbi files;
    MDB_dbi owner;
};

static void
raw_open(struct raw_db *raw, const char *path)
{
    assert(mdb_env_create(&raw->env) == MDB_SUCCESS);
    assert(mdb_env_set_maxdbs(raw->env, 4) == MDB_SUCCESS);
    assert(mdb_env_set_mapsize(raw->env, 4 * 1024 * 1024) == MDB_SUCCESS);
    assert(mdb_env_open(raw->env, path, 0, 0644) == MDB_SUCCESS);
    assert(mdb_txn_begin(raw->env, NULL, 0, &raw->txn) == MDB_SUCCESS);
    assert(mdb_dbi_open(raw->txn, NULL, 0, &raw->main) == MDB_SUCCESS);
    assert(mdb_dbi_open(raw->txn, "files", 0, &raw->files) == MDB_SUCCESS);
    assert(mdb_dbi_open(raw->txn, "file_owner", 0, &raw->owner) == MDB_SUCCESS);
}

static void
raw_close(struct raw_db *raw)
{
    assert(mdb_txn_commit(raw->txn) == MDB_SUCCESS);
    mdb_env_close(raw->env);
}

static void
raw_put(struct raw_db *raw, MDB_dbi dbi, const char *key, const char *value)
{
    MDB_val k = {strlen(key), (void *)key};
    MDB_val v = {strlen(value), (void *)value};
    assert(mdb_put(raw->txn, dbi, &k, &v, 0) == MDB_SUCCESS);
}

static void
raw_del(struct raw_db *raw, MDB_dbi dbi, const char *key)
{
    MDB_val k = {strlen(key), (void *)key};
    assert(mdb_del(raw->txn, dbi, &k, NULL) == MDB_SUCCESS);
}

static struct db_handle *
db_with_two_packages(char *path)
{
    struct db_handle *db = fs_open_db(path);
    assert(db);
    const char *fa[] = {"/usr/bin/a", "/usr/share/a/data"};
    const char *fb[] = {"/usr/bin/b"};
    struct package *a =
        fs_make_package("a", "1", NULL, NULL, 0, NULL, 0, fa, 2, true);
    struct package *b =
        fs_make_package("b", "1", NULL, NULL, 0, NULL, 0, fb, 1, true);
    struct package *empty =
        fs_make_package("empty", "1", NULL, NULL, 0, NULL, 0, NULL, 0, true);
    assert(db_add(db, a));
    assert(db_add(db, b));
    assert(db_add(db, empty));
    package_free(a);
    package_free(b);
    package_free(empty);
    return db;
}

static bool
has_issue(const struct db_check_issue *issues, int count, db_check_kind_t kind,
          const char *pkg, const char *path, const char *other)
{
    for (int i = 0; i < count; i++)
    {
        const struct db_check_issue *issue =
            db_check_issue_at(issues, count, i);
        const char *p = db_check_issue_pkg_name(issue);
        const char *f = db_check_issue_path(issue);
        const char *o = db_check_issue_other(issue);
        if (db_check_issue_kind(issue) != kind)
            continue;
        if ((pkg == NULL) != (p == NULL) || (pkg && strcmp(pkg, p) != 0))
            continue;
        if ((path == NULL) != (f == NULL) || (path && strcmp(path, f) != 0))
            continue;
        if ((other == NULL) != (o == NULL) || (other && strcmp(other, o) != 0))
            continue;
        return true;
    }
    return false;
}

void
test_db_check_reports_nothing_for_consistent_database(void)
{
    char path[PATH_MAX];
    struct db_handle *db = db_with_two_packages(path);

    int count = -1;
    struct db_check_issue *issues = db_check(db, &count);
    assert(issues == NULL && count == 0);

    const char *fc[] = {"/usr/bin/a", "/usr/bin/c"};
    struct package *replaced =
        fs_make_package("a", "2", NULL, NULL, 0, NULL, 0, fc, 2, true);
    assert(db_add(db, replaced));
    package_free(replaced);
    assert(db_set_hold(db, "b", true));
    assert(db_remove(db, "b"));
    issues = db_check(db, &count);
    assert(issues == NULL && count == 0);

    assert(db_check(NULL, &count) == NULL && count == 0);
    assert(db_check_issue_at(NULL, 0, 0) == NULL);
    db_check_free(NULL, 0);

    db_close(db);
    db = db_open_readonly(path);
    assert(db);
    issues = db_check(db, &count);
    assert(issues == NULL && count == 0);

    fs_close_db(db, path);
    printf("test_db_check_reports_nothing_for_consistent_database: PASS\n");
}

void
test_db_check_reports_ownership_inconsistencies(void)
{
    char path[PATH_MAX];
    struct db_handle *db = db_with_two_packages(path);
    db_close(db);

    struct raw_db raw;
    raw_open(&raw, path);
    raw_del(&raw, raw.owner, "/usr/share/a/data");
    raw_put(&raw, raw.owner, "/usr/bin/b", "a");
    raw_put(&raw, raw.owner, "/stray/file", "b");
    raw_put(&raw, raw.owner, "/ghost/file", "ghost");
    raw_close(&raw);

    db = db_open(path);
    assert(db);
    int count = 0;
    struct db_check_issue *issues = db_check(db, &count);
    assert(issues && count == 5);

    assert(has_issue(issues, count, DB_CHECK_FILE_NOT_OWNED, "a",
                     "/usr/share/a/data", NULL));
    assert(has_issue(issues, count, DB_CHECK_FILE_OWNED_BY_OTHER, "b",
                     "/usr/bin/b", "a"));
    assert(has_issue(issues, count, DB_CHECK_OWNER_NOT_LISTED, "a",
                     "/usr/bin/b", NULL));
    assert(has_issue(issues, count, DB_CHECK_OWNER_NOT_LISTED, "b",
                     "/stray/file", NULL));
    assert(has_issue(issues, count, DB_CHECK_OWNER_WITHOUT_PACKAGE, "ghost",
                     "/ghost/file", NULL));
    assert(db_check_issue_at(issues, count, count) == NULL);
    assert(db_check_issue_at(issues, count, -1) == NULL);

    db_check_free(issues, count);
    fs_close_db(db, path);
    printf("test_db_check_reports_ownership_inconsistencies: PASS\n");
}

void
test_db_check_reports_record_inconsistencies(void)
{
    char path[PATH_MAX];
    struct db_handle *db = db_with_two_packages(path);

    struct package *aliased = fs_make_package("real-name", "1", NULL, NULL, 0,
                                              NULL, 0, NULL, 0, true);
    char *json = package_to_json(aliased);
    assert(json);
    package_free(aliased);
    db_close(db);

    struct raw_db raw;
    raw_open(&raw, path);
    raw_put(&raw, raw.main, "alias", json);
    raw_put(&raw, raw.main, "broken", "{not json");
    raw_del(&raw, raw.main, "b");
    raw_put(&raw, raw.files, "orphan-list", "/orphan/file\n");
    raw_close(&raw);
    free(json);

    db = db_open(path);
    assert(db);
    int count = 0;
    struct db_check_issue *issues = db_check(db, &count);
    assert(issues && count == 5);

    assert(has_issue(issues, count, DB_CHECK_NAME_MISMATCH, "alias", NULL,
                     "real-name"));
    assert(has_issue(issues, count, DB_CHECK_CORRUPT_RECORD, "broken", NULL,
                     NULL));
    assert(has_issue(issues, count, DB_CHECK_FILES_WITHOUT_PACKAGE, "b", NULL,
                     NULL));
    assert(has_issue(issues, count, DB_CHECK_FILES_WITHOUT_PACKAGE,
                     "orphan-list", NULL, NULL));
    assert(has_issue(issues, count, DB_CHECK_OWNER_WITHOUT_PACKAGE, "b",
                     "/usr/bin/b", NULL));

    db_check_free(issues, count);
    fs_close_db(db, path);
    printf("test_db_check_reports_record_inconsistencies: PASS\n");
}
