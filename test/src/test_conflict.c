// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "helpers.h"

void
test_conflict_direct_new_vs_installed(void)
{
    struct package_metadata *new_pkg   = make_pkg("new", NODEPS, (const char *[]){"old"}, 1, NOPROVIDES, NOREPLACES);
    struct package_metadata *old_pkg   = make_pkg("old", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *unrelated = make_pkg("unrelated", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, new_pkg)   == DEP_OK);
    assert(dep_graph_add(g, old_pkg)   == DEP_OK);
    assert(dep_graph_add(g, unrelated) == DEP_OK);

    const char *installed[] = { "old", "unrelated" };
    char **breaks = NULL;
    size_t break_count = 0;
    assert(dep_graph_find_breaks(g, "new", installed, 2, &breaks, &break_count) == DEP_OK);
    assert(break_count == 1);
    assert(strcmp(breaks[0], "old") == 0);
    free(breaks);

    dep_graph_free(g);
    package_metadata_free(new_pkg);
    package_metadata_free(old_pkg);
    package_metadata_free(unrelated);
    printf("test_conflict_direct_new_vs_installed: PASS\n");
}

void
test_conflict_reverse_installed_vs_new(void)
{
    // Installed package declares conflict against the new package
    struct package_metadata *new_pkg  = make_pkg("new",  NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *inst_pkg = make_pkg("inst", NODEPS, (const char *[]){"new"}, 1, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, new_pkg)  == DEP_OK);
    assert(dep_graph_add(g, inst_pkg) == DEP_OK);

    const char *installed[] = { "inst" };
    char **breaks = NULL;
    size_t break_count = 0;
    assert(dep_graph_find_breaks(g, "new", installed, 1, &breaks, &break_count) == DEP_OK);
    assert(break_count == 1);
    assert(strcmp(breaks[0], "inst") == 0);
    free(breaks);

    dep_graph_free(g);
    package_metadata_free(new_pkg);
    package_metadata_free(inst_pkg);
    printf("test_conflict_reverse_installed_vs_new: PASS\n");
}

void
test_conflict_via_new_provides(void)
{
    // new provides "virtual"; installed conflicts with "virtual"
    struct package_metadata *new_pkg  = make_pkg("new",  NODEPS, NOCONFLICTS, (const char *[]){"virtual"}, 1, NOREPLACES);
    struct package_metadata *inst_pkg = make_pkg("inst", NODEPS, (const char *[]){"virtual"}, 1, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, new_pkg)  == DEP_OK);
    assert(dep_graph_add(g, inst_pkg) == DEP_OK);

    const char *installed[] = { "inst" };
    char **breaks = NULL;
    size_t break_count = 0;
    assert(dep_graph_find_breaks(g, "new", installed, 1, &breaks, &break_count) == DEP_OK);
    assert(break_count == 1);
    assert(strcmp(breaks[0], "inst") == 0);
    free(breaks);

    dep_graph_free(g);
    package_metadata_free(new_pkg);
    package_metadata_free(inst_pkg);
    printf("test_conflict_via_new_provides: PASS\n");
}

void
test_conflict_via_installed_provides(void)
{
    // new conflicts with "virtual"; installed provides "virtual"
    struct package_metadata *new_pkg  = make_pkg("new",  NODEPS, (const char *[]){"virtual"}, 1, NOPROVIDES, NOREPLACES);
    struct package_metadata *inst_pkg = make_pkg("inst", NODEPS, NOCONFLICTS, (const char *[]){"virtual"}, 1, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, new_pkg)  == DEP_OK);
    assert(dep_graph_add(g, inst_pkg) == DEP_OK);

    const char *installed[] = { "inst" };
    char **breaks = NULL;
    size_t break_count = 0;
    assert(dep_graph_find_breaks(g, "new", installed, 1, &breaks, &break_count) == DEP_OK);
    assert(break_count == 1);
    assert(strcmp(breaks[0], "inst") == 0);
    free(breaks);

    dep_graph_free(g);
    package_metadata_free(new_pkg);
    package_metadata_free(inst_pkg);
    printf("test_conflict_via_installed_provides: PASS\n");
}

void
test_no_conflicts(void)
{
    struct package_metadata *pkg   = make_pkg("pkg",   NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    struct package_metadata *other = make_pkg("other", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

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

void
test_breaks_empty_installed(void)
{
    struct package_metadata *pkg = make_pkg("pkg", NODEPS, (const char *[]){"anything"}, 1, NOPROVIDES, NOREPLACES);

    struct dep_graph *g = dep_graph_new();
    assert(dep_graph_add(g, pkg) == DEP_OK);

    char **breaks = NULL;
    size_t break_count = 0;
    assert(dep_graph_find_breaks(g, "pkg", NULL, 0, &breaks, &break_count) == DEP_OK);
    assert(break_count == 0);
    assert(breaks == NULL);

    dep_graph_free(g);
    package_metadata_free(pkg);
    printf("test_breaks_empty_installed: PASS\n");
}

void
test_breaks_unknown_pkg(void)
{
    struct dep_graph *g = dep_graph_new();

    char **breaks = NULL;
    size_t break_count = 0;
    assert(dep_graph_find_breaks(g, "ghost", NULL, 0, &breaks, &break_count) == DEP_ERR_MISSING);

    dep_graph_free(g);
    printf("test_breaks_unknown_pkg: PASS\n");
}
