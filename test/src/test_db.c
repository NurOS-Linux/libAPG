// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/db.h>
#include <apg/package.h>

#include "fs_helpers.h"

static void
add_pkg(struct db_handle *db, struct package *pkg)
{
    assert(db_add(db, pkg));
    package_free(pkg);
}

void
test_db_list_empty_and_populated(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    int count = -1;
    struct package **list = db_list(db, &count);
    assert(count == 0);
    free(list);

    const char *files[] = {"usr/bin/a", "usr/share/a/readme"};
    add_pkg(db, fs_make_package("a", "1.0", "first", NULL, 0, NULL, 0, files, 2,
                                true));
    add_pkg(db, fs_make_package("b", "2.0", "second", NULL, 0, NULL, 0, NULL, 0,
                                false));

    list = db_list(db, &count);
    assert(list && count == 2);
    bool seen_a = false, seen_b = false;
    for (int i = 0; i < count; i++)
    {
        if (strcmp(list[i]->meta->name, "a") == 0)
        {
            seen_a = true;
            assert(list[i]->package_files.count == 2);
            assert(list[i]->installed_by_hand);
        }
        if (strcmp(list[i]->meta->name, "b") == 0)
        {
            seen_b = true;
            assert(list[i]->package_files.count == 0);
            assert(!list[i]->installed_by_hand);
        }
        package_free(list[i]);
    }
    free(list);
    assert(seen_a && seen_b);

    fs_close_db(db, path);
    printf("test_db_list_empty_and_populated: PASS\n");
}

void
test_db_search_matches_name_and_description(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    add_pkg(db, fs_make_package("libfoo", "1.0", "Foo library", NULL, 0, NULL,
                                0, NULL, 0, true));
    add_pkg(db, fs_make_package("bar", "1.0", "uses the FOO engine", NULL, 0,
                                NULL, 0, NULL, 0, true));
    add_pkg(db, fs_make_package("baz", "1.0", NULL, NULL, 0, NULL, 0, NULL, 0,
                                true));

    int count = 0;
    struct package **res = db_search(db, "foo", &count);
    assert(res && count == 2);
    for (int i = 0; i < count; i++)
        package_free(res[i]);
    free(res);

    res = db_search(db, "BAZ", &count);
    assert(res && count == 1);
    assert(strcmp(res[0]->meta->name, "baz") == 0);
    package_free(res[0]);
    free(res);

    res = db_search(db, "nomatch", &count);
    assert(count == 0);
    free(res);

    res = db_search(db, "", &count);
    assert(res == NULL && count == 0);

    res = db_search(db, NULL, &count);
    assert(res == NULL && count == 0);

    fs_close_db(db, path);
    printf("test_db_search_matches_name_and_description: PASS\n");
}

void
test_db_stats_counts_packages_and_files(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    struct db_stats st;
    assert(db_stats(db, &st));
    assert(st.package_count == 0 && st.file_count == 0);

    const char *fa[] = {"usr/bin/a", "usr/lib/a.so", "etc/a.conf"};
    const char *fb[] = {"usr/bin/b"};
    add_pkg(db, fs_make_package("a", "1", NULL, NULL, 0, NULL, 0, fa, 3, true));
    add_pkg(db, fs_make_package("b", "1", NULL, NULL, 0, NULL, 0, fb, 1, true));

    assert(db_stats(db, &st));
    assert(st.package_count == 2);
    assert(st.file_count == 4);

    assert(db_remove(db, "a"));
    assert(db_stats(db, &st));
    assert(st.package_count == 1);
    assert(st.file_count == 1);

    assert(!db_stats(db, NULL));
    assert(!db_stats(NULL, &st));

    fs_close_db(db, path);
    printf("test_db_stats_counts_packages_and_files: PASS\n");
}

void
test_db_dependents_direct_and_via_provides(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    const char *prov[] = {"libvirt"};
    const char *dep_core[] = {"core"};
    const char *dep_virt[] = {"libvirt"};
    const char *dep_core_ver[] = {"core>=1.0"};
    add_pkg(db, fs_make_package("core", "1.0", NULL, NULL, 0, prov, 1, NULL, 0,
                                true));
    add_pkg(db, fs_make_package("app1", "1.0", NULL, dep_core, 1, NULL, 0, NULL,
                                0, true));
    add_pkg(db, fs_make_package("app2", "1.0", NULL, dep_virt, 1, NULL, 0, NULL,
                                0, true));
    add_pkg(db, fs_make_package("app3", "1.0", NULL, dep_core_ver, 1, NULL, 0,
                                NULL, 0, true));
    add_pkg(db, fs_make_package("lonely", "1.0", NULL, NULL, 0, NULL, 0, NULL,
                                0, true));

    int count = 0;
    char **deps = db_get_dependents(db, "core", &count);
    assert(count == 3);
    assert(fs_has_name(deps, count, "app1"));
    assert(fs_has_name(deps, count, "app2"));
    assert(fs_has_name(deps, count, "app3"));
    assert(!fs_has_name(deps, count, "core"));
    fs_free_names(deps, count);

    deps = db_get_dependents(db, "lonely", &count);
    assert(count == 0);
    free(deps);

    deps = db_get_dependents(db, "does-not-exist", &count);
    assert(count == 0);
    free(deps);

    deps = db_get_dependents(db, NULL, &count);
    assert(deps == NULL && count == 0);

    fs_close_db(db, path);
    printf("test_db_dependents_direct_and_via_provides: PASS\n");
}

void
test_db_orphans_respect_dependencies_and_flags(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    const char *dep_lib[] = {"lib"};
    const char *dep_virt[] = {"virt"};
    const char *prov_virt[] = {"virt"};
    add_pkg(db, fs_make_package("app", "1", NULL, dep_lib, 1, NULL, 0, NULL, 0,
                                true));
    add_pkg(db, fs_make_package("lib", "1", NULL, NULL, 0, NULL, 0, NULL, 0,
                                false));
    add_pkg(db, fs_make_package("stray", "1", NULL, NULL, 0, NULL, 0, NULL, 0,
                                false));
    add_pkg(db, fs_make_package("manual", "1", NULL, NULL, 0, NULL, 0, NULL, 0,
                                true));
    add_pkg(db, fs_make_package("needs-virt", "1", NULL, dep_virt, 1, NULL, 0,
                                NULL, 0, true));
    add_pkg(db, fs_make_package("provider", "1", NULL, NULL, 0, prov_virt, 1,
                                NULL, 0, false));

    int count = 0;
    char **orph = db_get_orphans(db, &count);
    assert(count == 1);
    assert(strcmp(orph[0], "stray") == 0);
    fs_free_names(orph, count);

    assert(db_set_installed_by_hand(db, "stray", true));
    orph = db_get_orphans(db, &count);
    assert(count == 0);
    free(orph);

    assert(db_remove(db, "app"));
    orph = db_get_orphans(db, &count);
    assert(count == 1);
    assert(strcmp(orph[0], "lib") == 0);
    fs_free_names(orph, count);

    fs_close_db(db, path);
    printf("test_db_orphans_respect_dependencies_and_flags: PASS\n");
}

void
test_db_set_hold_persists_and_validates(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    add_pkg(db, fs_make_package("held-pkg", "1", NULL, NULL, 0, NULL, 0, NULL,
                                0, true));

    struct package *p = db_get(db, "held-pkg");
    assert(p && !p->held);
    package_free(p);

    assert(db_set_hold(db, "held-pkg", true));
    p = db_get(db, "held-pkg");
    assert(p && p->held && p->installed_by_hand);
    package_free(p);

    assert(db_set_hold(db, "held-pkg", false));
    p = db_get(db, "held-pkg");
    assert(p && !p->held);
    package_free(p);

    assert(!db_set_hold(db, "missing", true));
    assert(!db_set_hold(NULL, "held-pkg", true));
    assert(!db_set_hold(db, NULL, true));

    assert(db_set_hold(db, "held-pkg", true));
    db_close(db);
    db = db_open_readonly(path);
    assert(db);
    p = db_get(db, "held-pkg");
    assert(p && p->held);
    package_free(p);
    assert(!db_set_hold(db, "held-pkg", false));

    fs_close_db(db, path);
    printf("test_db_set_hold_persists_and_validates: PASS\n");
}

struct hook_log
{
    int pre_add;
    int post_add;
    int pre_remove;
    int post_remove;
    char last_name[64];
};

static void
hook_pre(db_op_t op, const char *name, void *userdata)
{
    struct hook_log *log = userdata;
    if (op == DB_OP_ADD)
        log->pre_add++;
    else
        log->pre_remove++;
    snprintf(log->last_name, sizeof(log->last_name), "%s", name);
}

static void
hook_post(db_op_t op, const char *name, void *userdata)
{
    struct hook_log *log = userdata;
    if (op == DB_OP_ADD)
        log->post_add++;
    else
        log->post_remove++;
    snprintf(log->last_name, sizeof(log->last_name), "%s", name);
}

void
test_db_hooks_fire_on_add_and_remove(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    struct hook_log log = {0};
    struct db_hooks hooks = {
        .pre = hook_pre, .post = hook_post, .userdata = &log};
    db_set_hooks(db, &hooks);

    add_pkg(db, fs_make_package("hooked", "1", NULL, NULL, 0, NULL, 0, NULL, 0,
                                true));
    assert(log.pre_add == 1 && log.post_add == 1);
    assert(strcmp(log.last_name, "hooked") == 0);

    assert(db_remove(db, "hooked"));
    assert(log.pre_remove == 1 && log.post_remove == 1);

    assert(!db_remove(db, "hooked"));

    db_set_hooks(db, NULL);
    struct db_hooks none = {0};
    db_set_hooks(db, &none);
    add_pkg(db, fs_make_package("quiet", "1", NULL, NULL, 0, NULL, 0, NULL, 0,
                                true));
    assert(log.pre_add == 1 && log.post_add == 1);

    fs_close_db(db, path);
    printf("test_db_hooks_fire_on_add_and_remove: PASS\n");
}

void
test_db_owner_follows_add_and_remove(void)
{
    char path[PATH_MAX];
    struct db_handle *db = fs_open_db(path);
    assert(db);

    const char *files[] = {"usr/bin/tool", "etc/tool.conf"};
    add_pkg(db, fs_make_package("tool", "1", NULL, NULL, 0, NULL, 0, files, 2,
                                true));

    char *owner = db_owner(db, "usr/bin/tool");
    assert(owner && strcmp(owner, "tool") == 0);
    free(owner);

    assert(db_owner(db, "usr/bin/other") == NULL);
    assert(db_owner(db, NULL) == NULL);

    assert(db_remove(db, "tool"));
    assert(db_owner(db, "usr/bin/tool") == NULL);
    assert(db_owner(db, "etc/tool.conf") == NULL);

    fs_close_db(db, path);
    printf("test_db_owner_follows_add_and_remove: PASS\n");
}
