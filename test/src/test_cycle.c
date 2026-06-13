// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <assert.h>

#include "helpers.h"

void
test_self_cycle(void)
{
    struct package_metadata *a = make_pkg("a", (const char *[]){"a"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_has_cycle(g));

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_ERR_CYCLE);

    dep_graph_free(g);
    package_metadata_free(a);
    printf("test_self_cycle: PASS\n");
}

void
test_two_node_cycle(void)
{
    struct package_metadata *a = make_pkg("a", (const char *[]){"b"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *b = make_pkg("b", (const char *[]){"a"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, b) == DEP_OK);
    assert(dep_graph_has_cycle(g));

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_ERR_CYCLE);

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_two_node_cycle: PASS\n");
}

void
test_three_way_cycle(void)
{
    // a -> b -> c -> a
    struct package_metadata *a = make_pkg("a", (const char *[]){"b"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *b = make_pkg("b", (const char *[]){"c"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *c = make_pkg("c", (const char *[]){"a"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, b) == DEP_OK);
    assert(dep_graph_add(g, c) == DEP_OK);
    assert(dep_graph_has_cycle(g));

    char **order = NULL;
    size_t count = 0;
    assert(dep_graph_resolve(g, "a", &order, &count) == DEP_ERR_CYCLE);

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    package_metadata_free(c);
    printf("test_three_way_cycle: PASS\n");
}

void
test_has_cycle_empty(void)
{
    struct dep_graph *g = dep_graph_new();
    assert(!dep_graph_has_cycle(g));
    dep_graph_free(g);
    printf("test_has_cycle_empty: PASS\n");
}

void
test_has_cycle_forest(void)
{
    // Two independent DAGs in one graph — no cycle
    struct package_metadata *a = make_pkg("a", (const char *[]){"b"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *b =
        make_pkg("b", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *x = make_pkg("x", (const char *[]){"y"}, 1,
                                          NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *y =
        make_pkg("y", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, a) == DEP_OK);
    assert(dep_graph_add(g, b) == DEP_OK);
    assert(dep_graph_add(g, x) == DEP_OK);
    assert(dep_graph_add(g, y) == DEP_OK);
    assert(!dep_graph_has_cycle(g));

    dep_graph_free(g);
    package_metadata_free(a);
    package_metadata_free(b);
    package_metadata_free(x);
    package_metadata_free(y);
    printf("test_has_cycle_forest: PASS\n");
}
