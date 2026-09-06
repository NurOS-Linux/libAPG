// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <apg/config.h>
#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>
#include <apg/version.h>

static struct db_handle *
open_tmp_db(char *buf)
{
    strcpy(buf, "/tmp/apg-test-XXXXXX");
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

static struct package *
simple_pkg(const char *name)
{
    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta->name = strdup(name);
    pkg->meta->version = strdup("1.0");
    pkg->pkg_path = strdup("/nonexistent/test.pkg");
    pkg->installed_by_hand = true;
    return pkg;
}

void
test_policy_unsigned(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    install_policy p = {.require_signature = true,
                        .keyring_dir = "/nonexistent/keys"};
    trans_set_policy(trans, &p);

    assert(trans_commit(trans, "/tmp") == TRANS_ERR_UNSIGNED);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_policy_unsigned: PASS\n");
}

void
test_policy_no_sig_required(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    install_policy p = {.require_signature = false};
    trans_set_policy(trans, &p);

    assert(trans_commit(trans, "/tmp") == TRANS_ERR_INSTALL_FAILED);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_policy_no_sig_required: PASS\n");
}

void
test_policy_clear(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    install_policy p = {.require_signature = true,
                        .keyring_dir = "/nonexistent/keys"};
    trans_set_policy(trans, &p);
    trans_set_policy(trans, NULL);

    assert(trans_commit(trans, "/tmp") == TRANS_ERR_INSTALL_FAILED);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_policy_clear: PASS\n");
}

static struct package *
pkg_with_missing_dep(const char *name)
{
    struct package *pkg = simple_pkg(name);
    pkg->meta->dependencies.count = 1;
    pkg->meta->dependencies.items = malloc(sizeof(struct dep_constraint));
    pkg->meta->dependencies.items[0] = dep_constraint_parse("nonexistent-lib");
    return pkg;
}

void
test_policy_missing_dep_rejected_by_default(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = pkg_with_missing_dep("glibc");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);

    assert(trans_prepare(trans) == TRANS_ERR_MISSING_DEP);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_policy_missing_dep_rejected_by_default: PASS\n");
}

void
test_policy_skip_dependency_check_allows_missing_dep(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = pkg_with_missing_dep("glibc");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);

    install_policy p = {.skip_dependency_check = true};
    trans_set_policy(trans, &p);

    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_plan_count(trans) == 1);
    assert(strcmp(trans_step_pkg_name(trans_plan_at(trans, 0)), "glibc") == 0);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_policy_skip_dependency_check_allows_missing_dep: PASS\n");
}
