// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "helpers.h"
#include <apg/version.h>

void
test_linear_resolve(void)
{
    // a -> b -> c, expected install order: c, b, a
    struct package_metadata *a = make_pkg("a", (const char *[]){"b"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *b = make_pkg("b", (const char *[]){"c"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *c =
        make_pkg("c", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(g);
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, b) == DEP_OK);
    assert(dep_graph_add(g, c) == DEP_OK);
    assert(!dep_graph_has_cycle(g));

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_OK);
    assert(count == 3);
    assert(strcmp(order[0], "c") == 0);
    assert(strcmp(order[1], "b") == 0);
    assert(strcmp(order[2], "a") == 0);
    free(order);

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    package_metadata_free(c);
    printf("test_linear_resolve: PASS\n");
}

void
test_single_node_resolve(void)
{
    struct package_metadata *p =
        make_pkg("solo", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, p) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "solo", &order, &count) == DEP_OK);
    assert(count == 1);
    assert(strcmp(order[0], "solo") == 0);
    free(order);

    dep_graph_free(g);
    package_metadata_free(p);
    printf("test_single_node_resolve: PASS\n");
}

void
test_diamond_resolve(void)
{
    // app -> lib-a, lib-b -> base; base must appear exactly once, first
    struct package_metadata *app =
        make_pkg("app", (const char *[]){"lib-a", "lib-b"}, 2, NOCONFLICTS,
                 NOPROVIDES, NOREPLACES);
    struct package_metadata *liba =
        make_pkg("lib-a", (const char *[]){"base"}, 1, NOCONFLICTS, NOPROVIDES,
                 NOREPLACES);
    struct package_metadata *libb =
        make_pkg("lib-b", (const char *[]){"base"}, 1, NOCONFLICTS, NOPROVIDES,
                 NOREPLACES);
    struct package_metadata *base =
        make_pkg("base", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, app) == DEP_OK);
    assert(dep_graph_add(g, liba) == DEP_OK);
    assert(dep_graph_add(g, libb) == DEP_OK);
    assert(dep_graph_add(g, base) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "app", &order, &count) == DEP_OK);
    assert(count == 4);
    assert(strcmp(order[0], "base") == 0);
    assert(strcmp(order[count - 1], "app") == 0);
    free(order);

    dep_graph_free(g);
    package_metadata_free(app);
    package_metadata_free(liba);
    package_metadata_free(libb);
    package_metadata_free(base);
    printf("test_diamond_resolve: PASS\n");
}

void
test_duplicate_add(void)
{
    struct package_metadata *a =
        make_pkg("a", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *b = make_pkg("b", (const char *[]){"a"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, a) == DEP_OK); // second add must be a no-op
    assert(dep_graph_add(g, b) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "b", &order, &count) == DEP_OK);
    assert(count == 2);
    assert(strcmp(order[0], "a") == 0);
    assert(strcmp(order[1], "b") == 0);
    free(order);

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_duplicate_add: PASS\n");
}

void
test_resolve_unknown_root(void)
{
    struct dep_graph *g = dep_graph_new();

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "ghost", &order, &count) == DEP_ERR_MISSING);

    dep_graph_free(g);
    printf("test_resolve_unknown_root: PASS\n");
}

void
test_missing_transitive_dep(void)
{
    struct package_metadata *a = make_pkg("a", (const char *[]){"missing"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_ERR_MISSING);

    dep_graph_free(g);
    package_metadata_free(a);
    printf("test_missing_transitive_dep: PASS\n");
}

void
test_provides_resolution(void)
{
    struct package_metadata *app =
        make_pkg("app", (const char *[]){"libfoo"}, 1, NOCONFLICTS, NOPROVIDES,
                 NOREPLACES);
    struct package_metadata *compat =
        make_pkg("lib-compat", NODEPS, NOCONFLICTS, (const char *[]){"libfoo"},
                 1, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, app) == DEP_OK);
    assert(dep_graph_add(g, compat) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "app", &order, &count) == DEP_OK);
    assert(count == 2);
    assert(strcmp(order[0], "lib-compat") == 0);
    assert(strcmp(order[1], "app") == 0);
    free(order);

    dep_graph_free(g);
    package_metadata_free(app);
    package_metadata_free(compat);
    printf("test_provides_resolution: PASS\n");
}

void
test_resolve_via_alias(void)
{
    // resolve() called with a provides alias, not the canonical name
    struct package_metadata *impl =
        make_pkg("impl", NODEPS, NOCONFLICTS, (const char *[]){"virtual-lib"},
                 1, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, impl) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "virtual-lib", &order, &count) == DEP_OK);
    assert(count == 1);
    assert(strcmp(order[0], "impl") == 0);
    free(order);

    dep_graph_free(g);
    package_metadata_free(impl);
    printf("test_resolve_via_alias: PASS\n");
}

void
test_replaces_resolution(void)
{
    // pkg-new replaces "old-pkg" (not in graph); app depends on "old-pkg"
    struct package_metadata *app =
        make_pkg("app", (const char *[]){"old-pkg"}, 1, NOCONFLICTS, NOPROVIDES,
                 NOREPLACES);
    struct package_metadata *pkg_new =
        make_pkg("pkg-new", NODEPS, NOCONFLICTS, NOPROVIDES,
                 (const char *[]){"old-pkg"}, 1);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, app) == DEP_OK);
    assert(dep_graph_add(g, pkg_new) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "app", &order, &count) == DEP_OK);
    assert(count == 2);
    assert(strcmp(order[0], "pkg-new") == 0);
    assert(strcmp(order[1], "app") == 0);
    free(order);

    dep_graph_free(g);
    package_metadata_free(app);
    package_metadata_free(pkg_new);
    printf("test_replaces_resolution: PASS\n");
}

void
test_version_constraint_satisfied(void)
{
    struct package_metadata *a = make_pkg("a", (const char *[]){"b >= 1.0"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *b =
        make_pkg("b", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    b->version = strdup("2.0");

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, b) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_OK);
    assert(count == 2);
    assert(strcmp(order[0], "b") == 0);
    assert(strcmp(order[1], "a") == 0);
    free(order);

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_version_constraint_satisfied: PASS\n");
}

void
test_version_constraint_unsatisfied(void)
{
    struct package_metadata *a = make_pkg("a", (const char *[]){"b >= 3.0"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *b =
        make_pkg("b", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    b->version = strdup("2.0");

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, b) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_ERR_VERSION);

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_version_constraint_unsatisfied: PASS\n");
}

void
test_version_exact_match(void)
{
    struct package_metadata *a =
        make_pkg("a", (const char *[]){"b == 2.1.0"}, 1, NOCONFLICTS,
                 NOPROVIDES, NOREPLACES);
    struct package_metadata *b =
        make_pkg("b", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    b->version = strdup("2.1.0");

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, b) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_OK);
    free(order);

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_version_exact_match: PASS\n");
}

void
test_epoch_higher_wins(void)
{
    assert(ver_compare("1:1.0", "0:9.9") > 0);
    assert(ver_compare("0:9.9", "1:1.0") < 0);
    assert(ver_satisfies("1:1.0", VER_OP_GT, "0:9.9"));
    assert(!ver_satisfies("0:9.9", VER_OP_GT, "1:1.0"));
    printf("test_epoch_higher_wins: PASS\n");
}

void
test_epoch_zero_implicit(void)
{
    assert(ver_compare("1:1.0", "2.0") > 0);
    assert(ver_compare("2.0", "1:1.0") < 0);
    assert(ver_compare("0:2.0", "2.0") == 0);
    printf("test_epoch_zero_implicit: PASS\n");
}

void
test_epoch_equal_falls_through(void)
{
    assert(ver_compare("1:2.0", "1:1.0") > 0);
    assert(ver_compare("1:1.0", "1:2.0") < 0);
    assert(ver_compare("1:1.0", "1:1.0") == 0);
    assert(ver_satisfies("1:2.0", VER_OP_GE, "1:1.0"));
    printf("test_epoch_equal_falls_through: PASS\n");
}
