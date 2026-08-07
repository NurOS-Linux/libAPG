// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <apg/audit.h>
#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>

static struct db_handle *
open_tmp_db(char *buf)
{
    strcpy(buf, "/tmp/apg-test-acc-XXXXXX");
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
test_trans_plan_accessors(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("plan-acc-pkg");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    assert(trans_plan_count(trans) == 1);
    const struct trans_step *step = trans_plan_at(trans, 0);
    assert(step);
    assert(trans_step_op(step) == TRANS_OP_INSTALL);
    assert(strcmp(trans_step_pkg_name(step), "plan-acc-pkg") == 0);
    assert(strcmp(trans_step_pkg_version(step), "1.0") == 0);
    assert(trans_step_explicit(step));
    assert(trans_plan_at(trans, 1) == NULL);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_trans_plan_accessors: PASS\n");
}

void
test_trans_conflict_accessors(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *old_pkg = simple_pkg("conflict-old");
    assert(db_add(db, old_pkg));

    struct package *new_pkg = simple_pkg("conflict-new");
    new_pkg->meta->conflicts.items = malloc(sizeof(char *));
    new_pkg->meta->conflicts.items[0] = strdup("conflict-old");
    new_pkg->meta->conflicts.count = 1;

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, new_pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_ERR_CONFLICT);

    assert(trans_conflict_count(trans) == 1);
    const struct trans_conflict *c = trans_conflict_at(trans, 0);
    assert(c);
    assert(strcmp(trans_conflict_pkg_name(c), "conflict-new") == 0);
    assert(strcmp(trans_conflict_conflicts_with(c), "conflict-old") == 0);
    assert(trans_conflict_at(trans, 1) == NULL);

    trans_free(trans);
    package_free(old_pkg);
    package_free(new_pkg);
    close_tmp_db(db, db_path);
    printf("test_trans_conflict_accessors: PASS\n");
}

void
test_db_verify_issue_accessors(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("verify-acc-pkg");
    pkg->package_files.items = malloc(sizeof(char *));
    pkg->package_files.items[0] = strdup("usr/bin/does-not-exist");
    pkg->package_files.count = 1;
    assert(db_add(db, pkg));

    int count = 0;
    struct db_verify_issue *issues = db_verify(db, "/", &count);
    assert(issues);
    assert(count == 1);

    const struct db_verify_issue *issue = db_verify_issue_at(issues, count, 0);
    assert(issue);
    assert(strcmp(db_verify_issue_pkg_name(issue), "verify-acc-pkg") == 0);
    assert(db_verify_issue_missing_count(issue) == 1);
    assert(strcmp(db_verify_issue_missing_file_at(issue, 0),
                  "usr/bin/does-not-exist") == 0);
    assert(db_verify_issue_missing_file_at(issue, 1) == NULL);
    assert(db_verify_issue_at(issues, count, 1) == NULL);

    db_verify_free(issues, count);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_db_verify_issue_accessors: PASS\n");
}

void
test_journal_entry_accessors(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("journal-acc-pkg");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    install_policy p = {.require_signature = false};
    trans_set_policy(trans, &p);
    assert(trans_commit(trans, "/tmp") == TRANS_ERR_INSTALL_FAILED);

    int count = 0;
    struct journal_entry **entries = audit_read_all(db, &count);
    assert(entries);
    assert(count >= 1);

    const struct journal_entry *e = entries[count - 1];
    assert(journal_entry_op(e) == JOURNAL_INSTALL);
    assert(strcmp(journal_entry_pkg_name(e), "journal-acc-pkg") == 0);
    assert(journal_entry_status(e) == JOURNAL_STATUS_FAILED);
    assert(journal_entry_explicit(e));
    assert(journal_entry_timestamp(e) > 0);

    journal_free_all(entries, count);
    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_journal_entry_accessors: PASS\n");
}

static struct package *
provider_pkg(const char *name, const char *version, const char *provides)
{
    struct package *pkg = simple_pkg(name);
    free(pkg->meta->version);
    pkg->meta->version = strdup(version);
    pkg->meta->provides.count = 1;
    pkg->meta->provides.items = malloc(sizeof(char *));
    pkg->meta->provides.items[0] = strdup(provides);
    return pkg;
}

void
test_trans_prefer_provider_overrides_installed_default(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *nginx_installed = provider_pkg("nginx", "1.0", "webserver");
    assert(db_add(db, nginx_installed));

    struct package *caddy = provider_pkg("caddy", "2.0", "webserver");
    struct package *app = simple_pkg("app");
    app->meta->dependencies.count = 1;
    app->meta->dependencies.items = malloc(sizeof(struct dep_constraint));
    app->meta->dependencies.items[0] = dep_constraint_parse("webserver >= 2.0");

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, caddy) == TRANS_OK);
    assert(trans_add_install(trans, app) == TRANS_OK);

    trans_prefer_provider(trans, "webserver", "caddy");
    assert(trans_prepare(trans) == TRANS_OK);

    assert(trans_plan_count(trans) == 2);
    assert(strcmp(trans_step_pkg_name(trans_plan_at(trans, 0)), "caddy") == 0);
    assert(strcmp(trans_step_pkg_name(trans_plan_at(trans, 1)), "app") == 0);

    trans_free(trans);
    package_free(caddy);
    package_free(app);
    package_free(nginx_installed);
    close_tmp_db(db, db_path);
    printf("test_trans_prefer_provider_overrides_installed_default: PASS\n");
}

void
test_trans_default_prefers_installed_provider(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *nginx_installed = provider_pkg("nginx", "1.0", "webserver");
    assert(db_add(db, nginx_installed));

    struct package *caddy = provider_pkg("caddy", "2.0", "webserver");
    struct package *app = simple_pkg("app");
    app->meta->dependencies.count = 1;
    app->meta->dependencies.items = malloc(sizeof(struct dep_constraint));
    app->meta->dependencies.items[0] = dep_constraint_parse("webserver >= 2.0");

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, caddy) == TRANS_OK);
    assert(trans_add_install(trans, app) == TRANS_OK);

    assert(trans_prepare(trans) != TRANS_OK);

    trans_free(trans);
    package_free(caddy);
    package_free(app);
    package_free(nginx_installed);
    close_tmp_db(db, db_path);
    printf("test_trans_default_prefers_installed_provider: PASS\n");
}

void
test_trans_upgrade_in_place_keeps_installed_preference(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *nginx_installed = provider_pkg("nginx", "1.0", "webserver");
    assert(db_add(db, nginx_installed));

    struct package *apache = provider_pkg("apache", "1.0", "webserver");
    struct package *nginx_upgrade = provider_pkg("nginx", "2.0", "webserver");
    struct package *app = simple_pkg("app");
    app->meta->dependencies.count = 1;
    app->meta->dependencies.items = malloc(sizeof(struct dep_constraint));
    app->meta->dependencies.items[0] = dep_constraint_parse("webserver >= 1.5");

    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, apache) == TRANS_OK);
    assert(trans_add_upgrade(trans, nginx_upgrade) == TRANS_OK);
    assert(trans_add_install(trans, app) == TRANS_OK);

    assert(trans_prepare(trans) == TRANS_OK);

    trans_free(trans);
    package_free(apache);
    package_free(nginx_upgrade);
    package_free(app);
    package_free(nginx_installed);
    close_tmp_db(db, db_path);
    printf("test_trans_upgrade_in_place_keeps_installed_preference: PASS\n");
}
