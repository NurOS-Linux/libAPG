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

static struct package *
pkg_with_dependency(const char *name, const char *dep_name)
{
    struct package *pkg = simple_pkg(name);
    pkg->meta->dependencies.count = 1;
    pkg->meta->dependencies.items = malloc(sizeof(struct dep_constraint));
    pkg->meta->dependencies.items[0] = dep_constraint_parse(dep_name);
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

void
test_dry_run_install_reports_ok_without_side_effects(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    trans_set_dry_run(trans, true);

    assert(trans_commit(trans, "/tmp") == TRANS_OK);
    assert(db_get(db, "foo") == NULL);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_dry_run_install_reports_ok_without_side_effects: PASS\n");
}

void
test_dry_run_still_reports_unsigned(void)
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
    trans_set_dry_run(trans, true);

    assert(trans_commit(trans, "/tmp") == TRANS_ERR_UNSIGNED);
    assert(db_get(db, "foo") == NULL);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_dry_run_still_reports_unsigned: PASS\n");
}

void
test_dry_run_does_not_count_towards_commit_limit(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    trans_set_dry_run(trans, true);
    assert(trans_commit(trans, "/tmp") == TRANS_OK);

    trans_set_dry_run(trans, false);
    assert(trans_commit(trans, "/tmp") != TRANS_ERR_ALREADY_COMMITTED);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_dry_run_does_not_count_towards_commit_limit: PASS\n");
}

void
test_policy_remove_blocked_by_dependents_by_default(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *libfoo = simple_pkg("libfoo");
    struct package *app = pkg_with_dependency("app", "libfoo");
    assert(db_add(db, libfoo));
    assert(db_add(db, app));

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_remove(trans, "libfoo") == TRANS_OK);

    assert(trans_prepare(trans) == TRANS_ERR_HAS_DEPENDENTS);
    assert(trans_blocked_remove_count(trans) == 1);

    const struct trans_blocked_remove *blocked =
        trans_blocked_remove_at(trans, 0);
    assert(strcmp(trans_blocked_remove_pkg_name(blocked), "libfoo") == 0);
    assert(trans_blocked_remove_dependent_count(blocked) == 1);
    assert(strcmp(trans_blocked_remove_dependent_at(blocked, 0), "app") == 0);

    trans_free(trans);
    package_free(libfoo);
    package_free(app);
    close_tmp_db(db, db_path);
    printf("test_policy_remove_blocked_by_dependents_by_default: PASS\n");
}

void
test_policy_skip_dependents_check_allows_blocked_remove(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *libfoo = simple_pkg("libfoo");
    struct package *app = pkg_with_dependency("app", "libfoo");
    assert(db_add(db, libfoo));
    assert(db_add(db, app));

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_remove(trans, "libfoo") == TRANS_OK);

    install_policy p = {.skip_dependents_check = true};
    trans_set_policy(trans, &p);

    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_blocked_remove_count(trans) == 0);
    assert(trans_plan_count(trans) == 1);
    assert(strcmp(trans_step_pkg_name(trans_plan_at(trans, 0)), "libfoo") == 0);

    trans_free(trans);
    package_free(libfoo);
    package_free(app);
    close_tmp_db(db, db_path);
    printf("test_policy_skip_dependents_check_allows_blocked_remove: PASS\n");
}

void
test_dry_run_remove_does_not_modify_db(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    assert(db_add(db, pkg));

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_remove(trans, "foo") == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    trans_set_dry_run(trans, true);
    assert(trans_commit(trans, "/tmp") == TRANS_OK);

    struct package *still_there = db_get(db, "foo");
    assert(still_there);
    package_free(still_there);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_dry_run_remove_does_not_modify_db: PASS\n");
}

static const struct trans_step *
find_step(struct apg_trans *trans, const char *name)
{
    size_t count = trans_plan_count(trans);
    for (size_t i = 0; i < count; i++)
    {
        const struct trans_step *step = trans_plan_at(trans, i);
        if (strcmp(trans_step_pkg_name(step), name) == 0)
            return step;
    }
    return NULL;
}

void
test_candidate_resolved_when_needed_and_marked_non_explicit(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *app = pkg_with_dependency("app", "libfoo");
    struct package *libfoo = simple_pkg("libfoo");

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, app) == TRANS_OK);
    assert(trans_add_candidate(trans, libfoo) == TRANS_OK);

    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_plan_count(trans) == 2);

    const struct trans_step *app_step = find_step(trans, "app");
    const struct trans_step *libfoo_step = find_step(trans, "libfoo");
    assert(app_step && trans_step_explicit(app_step));
    assert(libfoo_step && !trans_step_explicit(libfoo_step));

    trans_free(trans);
    package_free(app);
    package_free(libfoo);
    close_tmp_db(db, db_path);
    printf(
        "test_candidate_resolved_when_needed_and_marked_non_explicit: PASS\n");
}

void
test_candidate_not_planned_when_unneeded(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *app = simple_pkg("app");
    struct package *libfoo = simple_pkg("libfoo");

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, app) == TRANS_OK);
    assert(trans_add_candidate(trans, libfoo) == TRANS_OK);

    assert(trans_prepare(trans) == TRANS_OK);
    assert(trans_plan_count(trans) == 1);
    assert(find_step(trans, "app"));
    assert(!find_step(trans, "libfoo"));

    trans_free(trans);
    package_free(app);
    package_free(libfoo);
    close_tmp_db(db, db_path);
    printf("test_candidate_not_planned_when_unneeded: PASS\n");
}
