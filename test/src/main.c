// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <apg/package.h>
#include <apg/graph.h>

static struct package_metadata *
make_pkg(const char *name, const char **deps, int dep_count,
         const char **conflicts, int conflict_count,
         const char **provides, int provides_count)
{
    struct package_metadata *pkg = package_metadata_new();
    assert(pkg);
    pkg->name = strdup(name);

    pkg->dependencies.count = dep_count;
    if (dep_count > 0) {
        pkg->dependencies.items = malloc(dep_count * sizeof(char *));
        for (int i = 0; i < dep_count; i++)
            pkg->dependencies.items[i] = strdup(deps[i]);
    }

    pkg->conflicts.count = conflict_count;
    if (conflict_count > 0) {
        pkg->conflicts.items = malloc(conflict_count * sizeof(char *));
        for (int i = 0; i < conflict_count; i++)
            pkg->conflicts.items[i] = strdup(conflicts[i]);
    }

    pkg->provides.count = provides_count;
    if (provides_count > 0) {
        pkg->provides.items = malloc(provides_count * sizeof(char *));
        for (int i = 0; i < provides_count; i++)
            pkg->provides.items[i] = strdup(provides[i]);
    }

    return pkg;
}

static void
test_linear_resolve(void)
{
    // a -> b -> c  (a depends on b, b depends on c)
    // Expected install order: c, b, a
    struct package_metadata *a = make_pkg("a", (const char *[]){"b"}, 1, NULL, 0, NULL, 0);
    struct package_metadata *b = make_pkg("b", (const char *[]){"c"}, 1, NULL, 0, NULL, 0);
    struct package_metadata *c = make_pkg("c", NULL, 0, NULL, 0, NULL, 0);

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

static void
test_diamond_resolve(void)
{
    // app depends on lib-a and lib-b; both depend on base
    // Expected: base, lib-a, lib-b, app (base must come first, no duplicates)
    struct package_metadata *app   = make_pkg("app",   (const char *[]){"lib-a", "lib-b"}, 2, NULL, 0, NULL, 0);
    struct package_metadata *liba  = make_pkg("lib-a", (const char *[]){"base"}, 1, NULL, 0, NULL, 0);
    struct package_metadata *libb  = make_pkg("lib-b", (const char *[]){"base"}, 1, NULL, 0, NULL, 0);
    struct package_metadata *base  = make_pkg("base",  NULL, 0, NULL, 0, NULL, 0);

    struct dep_graph *g = dep_graph_new();
    assert(g);
    assert(dep_graph_add(g, app)  == DEP_OK);
    assert(dep_graph_add(g, liba) == DEP_OK);
    assert(dep_graph_add(g, libb) == DEP_OK);
    assert(dep_graph_add(g, base) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "app", &order, &count) == DEP_OK);
    // base must be first, app must be last
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

static void
test_cycle_detection(void)
{
    // a depends on b, b depends on a — cycle
    struct package_metadata *a = make_pkg("a", (const char *[]){"b"}, 1, NULL, 0, NULL, 0);
    struct package_metadata *b = make_pkg("b", (const char *[]){"a"}, 1, NULL, 0, NULL, 0);

    struct dep_graph *g = dep_graph_new();
    assert(g);
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, b) == DEP_OK);
    assert(dep_graph_has_cycle(g));

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_ERR_CYCLE);

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_cycle_detection: PASS\n");
}

static void
test_missing_dep(void)
{
    struct package_metadata *a = make_pkg("a", (const char *[]){"missing"}, 1, NULL, 0, NULL, 0);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_ERR_MISSING);

    dep_graph_free(g);
    package_metadata_free(a);
    printf("test_missing_dep: PASS\n");
}

static void
test_provides_resolution(void)
{
    // lib-compat provides "libfoo"; app depends on "libfoo"
    struct package_metadata *app    = make_pkg("app",        (const char *[]){"libfoo"}, 1, NULL, 0, NULL, 0);
    struct package_metadata *compat = make_pkg("lib-compat", NULL, 0, NULL, 0, (const char *[]){"libfoo"}, 1);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, app)    == DEP_OK);
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

static void
test_conflict_detection(void)
{
    // pkg-new conflicts with pkg-old; pkg-old is installed
    struct package_metadata *new_pkg = make_pkg("pkg-new", NULL, 0,
                                                 (const char *[]){"pkg-old"}, 1,
                                                 NULL, 0);
    struct package_metadata *old_pkg = make_pkg("pkg-old", NULL, 0, NULL, 0, NULL, 0);
    struct package_metadata *unrelated = make_pkg("unrelated", NULL, 0, NULL, 0, NULL, 0);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, new_pkg)   == DEP_OK);
    assert(dep_graph_add(g, old_pkg)   == DEP_OK);
    assert(dep_graph_add(g, unrelated) == DEP_OK);

    const char *installed[] = { "pkg-old", "unrelated" };
    char **breaks = NULL;
    size_t break_count = 0;
    assert(dep_graph_find_breaks(g, "pkg-new", installed, 2, &breaks, &break_count) == DEP_OK);
    assert(break_count == 1);
    assert(strcmp(breaks[0], "pkg-old") == 0);
    free(breaks);

    dep_graph_free(g);
    package_metadata_free(new_pkg);
    package_metadata_free(old_pkg);
    package_metadata_free(unrelated);
    printf("test_conflict_detection: PASS\n");
}

static void
test_no_conflicts(void)
{
    struct package_metadata *pkg = make_pkg("pkg", NULL, 0, NULL, 0, NULL, 0);
    struct package_metadata *other = make_pkg("other", NULL, 0, NULL, 0, NULL, 0);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, pkg)   == DEP_OK);
    assert(dep_graph_add(g, other) == DEP_OK);

    const char *installed[] = { "other" };
    char **breaks = NULL;
    size_t break_count = 0;
    assert(dep_graph_find_breaks(g, "pkg", installed, 1, &breaks, &break_count) == DEP_OK);
    assert(break_count == 0);
    assert(breaks == NULL);

    dep_graph_free(g);
    package_metadata_free(pkg);
    package_metadata_free(other);
    printf("test_no_conflicts: PASS\n");
}

int
main(void)
{
    test_linear_resolve();
    test_diamond_resolve();
    test_cycle_detection();
    test_missing_dep();
    test_provides_resolution();
    test_conflict_detection();
    test_no_conflicts();
    printf("All graph tests passed.\n");
    return 0;
}
