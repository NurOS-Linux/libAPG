// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>

#include "fs_helpers.h"

static struct package *
plain(const char *name, const char *version)
{
    struct package *p =
        fs_make_package(name, version, NULL, NULL, 0, NULL, 0, NULL, 0, true);
    p->pkg_path = strdup("/nonexistent/test.pkg");
    return p;
}

static struct package *
with_files(const char *name, const char **files, int count)
{
    struct package *p =
        fs_make_package(name, "1.0", NULL, NULL, 0, NULL, 0, files, count, true);
    p->pkg_path = strdup("/nonexistent/test.pkg");
    return p;
}

static void
store(struct db_handle *db, struct package *pkg)
{
    assert(db_add(db, pkg));
    package_free(pkg);
}

void
test_trans_held_upgrade_is_reported(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    store(db, plain("pinned", "1.0"));
    assert(db_set_hold(db, "pinned", true));

    struct package *newer = plain("pinned", "2.0");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_upgrade(trans, newer) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_ERR_HELD);

    assert(trans_held_pkg_count(trans) == 1);
    const struct trans_held_pkg *h = trans_held_pkg_at(trans, 0);
    assert(h);
    assert(strcmp(trans_held_pkg_name(h), "pinned") == 0);
    assert(trans_held_pkg_op(h) == TRANS_OP_UPGRADE);
    assert(trans_plan_count(trans) == 0);
    assert(trans_commit(trans, "/tmp") == TRANS_ERR_NOT_PREPARED);

    trans_free(trans);
    package_free(newer);
    fs_close_db(db, path);
    printf("test_trans_held_upgrade_is_reported: PASS\n");
}

void
test_trans_held_remove_is_reported_and_others_planned(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    store(db, plain("pinned", "1.0"));
    store(db, plain("free", "1.0"));
    assert(db_set_hold(db, "pinned", true));

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_remove(trans, "pinned") == TRANS_OK);
    assert(trans_add_remove(trans, "free") == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_ERR_HELD);

    assert(trans_held_pkg_count(trans) == 1);
    assert(trans_held_pkg_op(trans_held_pkg_at(trans, 0)) == TRANS_OP_REMOVE);
    assert(trans_plan_count(trans) == 1);
    assert(strcmp(trans_step_pkg_name(trans_plan_at(trans, 0)), "free") == 0);

    trans_free(trans);
    fs_close_db(db, path);
    printf("test_trans_held_remove_is_reported_and_others_planned: PASS\n");
}

void
test_trans_released_hold_allows_operation(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    store(db, plain("pinned", "1.0"));
    assert(db_set_hold(db, "pinned", true));
    assert(db_set_hold(db, "pinned", false));

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_remove(trans, "pinned") == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_held_pkg_count(trans) == 0);
    assert(trans_plan_count(trans) == 1);

    trans_free(trans);
    fs_close_db(db, path);
    printf("test_trans_released_hold_allows_operation: PASS\n");
}

void
test_trans_many_held_packages_are_all_reported(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    struct apg_trans *trans = trans_new(db);
    for (int i = 0; i < 9; i++)
    {
        char name[16];
        snprintf(name, sizeof(name), "held%d", i);
        store(db, plain(name, "1.0"));
        assert(db_set_hold(db, name, true));
        assert(trans_add_remove(trans, name) == TRANS_OK);
    }
    assert(trans_prepare(trans) == TRANS_ERR_HELD);
    assert(trans_held_pkg_count(trans) == 9);
    for (size_t i = 0; i < 9; i++)
        assert(trans_held_pkg_name(trans_held_pkg_at(trans, i)) != NULL);

    trans_free(trans);
    fs_close_db(db, path);
    printf("test_trans_many_held_packages_are_all_reported: PASS\n");
}

void
test_trans_file_conflict_is_reported(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    const char *owned[] = {"usr/bin/tool", "usr/share/tool/data"};
    store(db, with_files("owner", owned, 2));

    const char *wanted[] = {"usr/bin/tool", "usr/bin/other"};
    struct package *newcomer = with_files("newcomer", wanted, 2);
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, newcomer) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_ERR_FILE_CONFLICT);

    assert(trans_file_conflict_count(trans) == 1);
    const struct trans_file_conflict *fc = trans_file_conflict_at(trans, 0);
    assert(fc);
    assert(strcmp(trans_file_conflict_path(fc), "usr/bin/tool") == 0);
    assert(strcmp(trans_file_conflict_requested_by(fc), "newcomer") == 0);
    assert(strcmp(trans_file_conflict_owned_by(fc), "owner") == 0);

    trans_free(trans);
    package_free(newcomer);
    fs_close_db(db, path);
    printf("test_trans_file_conflict_is_reported: PASS\n");
}

void
test_trans_many_file_conflicts_are_all_reported(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    const char *files[] = {"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8"};
    store(db, with_files("owner", files, 9));

    struct package *newcomer = with_files("newcomer", files, 9);
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, newcomer) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_ERR_FILE_CONFLICT);
    assert(trans_file_conflict_count(trans) == 9);
    for (size_t i = 0; i < 9; i++)
    {
        const struct trans_file_conflict *fc = trans_file_conflict_at(trans, i);
        assert(strcmp(trans_file_conflict_owned_by(fc), "owner") == 0);
    }

    trans_free(trans);
    package_free(newcomer);
    fs_close_db(db, path);
    printf("test_trans_many_file_conflicts_are_all_reported: PASS\n");
}

void
test_trans_own_files_do_not_conflict(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    const char *files[] = {"usr/bin/tool"};
    store(db, with_files("tool", files, 1));

    struct package *again = with_files("tool", files, 1);
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, again) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_file_conflict_count(trans) == 0);

    trans_free(trans);
    package_free(again);
    fs_close_db(db, path);
    printf("test_trans_own_files_do_not_conflict: PASS\n");
}

void
test_trans_removed_owner_frees_its_files(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    const char *files[] = {"usr/bin/tool"};
    store(db, with_files("old-owner", files, 1));

    struct package *newcomer = with_files("newcomer", files, 1);
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_remove(trans, "old-owner") == TRANS_OK);
    assert(trans_add_install(trans, newcomer) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    trans_free(trans);
    package_free(newcomer);
    fs_close_db(db, path);
    printf("test_trans_removed_owner_frees_its_files: PASS\n");
}

void
test_trans_error_priority_held_before_file_conflict(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    const char *files[] = {"usr/bin/tool"};
    store(db, with_files("owner", files, 1));
    store(db, plain("pinned", "1.0"));
    assert(db_set_hold(db, "pinned", true));

    struct package *newcomer = with_files("newcomer", files, 1);
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, newcomer) == TRANS_OK);
    assert(trans_add_remove(trans, "pinned") == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_ERR_HELD);
    assert(trans_held_pkg_count(trans) == 1);
    assert(trans_file_conflict_count(trans) == 1);

    trans_free(trans);
    package_free(newcomer);
    fs_close_db(db, path);
    printf("test_trans_error_priority_held_before_file_conflict: PASS\n");
}

void
test_trans_accessors_handle_bad_arguments(void)
{
    assert(trans_plan_count(NULL) == 0);
    assert(trans_plan_at(NULL, 0) == NULL);
    assert(trans_conflict_count(NULL) == 0);
    assert(trans_conflict_at(NULL, 0) == NULL);
    assert(trans_file_conflict_count(NULL) == 0);
    assert(trans_file_conflict_at(NULL, 0) == NULL);
    assert(trans_held_pkg_count(NULL) == 0);
    assert(trans_held_pkg_at(NULL, 0) == NULL);
    assert(trans_blocked_remove_count(NULL) == 0);
    assert(trans_blocked_remove_at(NULL, 0) == NULL);

    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);
    struct apg_trans *trans = trans_new(db);
    assert(trans);
    assert(trans_plan_at(trans, 0) == NULL);
    assert(trans_plan_at(trans, 1000) == NULL);
    assert(trans_file_conflict_at(trans, 0) == NULL);
    assert(trans_held_pkg_at(trans, 0) == NULL);
    assert(trans_blocked_remove_at(trans, 0) == NULL);
    assert(trans_conflict_at(trans, 0) == NULL);

    assert(trans_add_install(trans, NULL) != TRANS_OK);
    assert(trans_add_upgrade(trans, NULL) != TRANS_OK);
    assert(trans_add_remove(trans, NULL) != TRANS_OK);
    assert(trans_add_install(NULL, NULL) != TRANS_OK);

    trans_free(trans);
    trans_free(NULL);
    fs_close_db(db, path);
    printf("test_trans_accessors_handle_bad_arguments: PASS\n");
}

void
test_trans_commit_state_machine(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);
    char *root = fs_mktmp("trans-root");

    struct apg_trans *trans = trans_new(db);
    assert(trans_commit(trans, root) == TRANS_ERR_NOT_PREPARED);
    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_plan_count(trans) == 0);
    assert(trans_commit(trans, root) == TRANS_OK);
    assert(trans_commit(trans, root) == TRANS_ERR_ALREADY_COMMITTED);

    trans_free(trans);
    fs_rmtree(root);
    free(root);
    fs_close_db(db, path);
    printf("test_trans_commit_state_machine: PASS\n");
}

void
test_trans_remove_of_unknown_package_does_not_corrupt_db(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);
    char *root = fs_mktmp("trans-unknown");

    store(db, plain("kept", "1.0"));

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_remove(trans, "never-installed") == TRANS_OK);
    trans_error_t prep = trans_prepare(trans);
    if (prep == TRANS_OK)
        (void)trans_commit(trans, root);

    struct package *kept = db_get(db, "kept");
    assert(kept);
    package_free(kept);
    int count = 0;
    struct package **all = db_list(db, &count);
    assert(count == 1);
    for (int i = 0; i < count; i++)
        package_free(all[i]);
    free(all);

    trans_free(trans);
    fs_rmtree(root);
    free(root);
    fs_close_db(db, path);
    printf("test_trans_remove_of_unknown_package_does_not_corrupt_db: PASS\n");
}
