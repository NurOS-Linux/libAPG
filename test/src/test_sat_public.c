// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "helpers.h"
#include <apg/graph.h>

void
test_sat_public_satisfiable(void)
{
    struct package_metadata *app =
        make_pkg("app", (const char *[]){"lib>=2.0.0"}, 1, NOCONFLICTS,
                 NOPROVIDES, NOREPLACES);
    struct package_metadata *lib1 =
        make_pkg("lib", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    free(lib1->version);
    lib1->version = strdup("1.0.0");
    struct package_metadata *lib2 =
        make_pkg("lib", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    free(lib2->version);
    lib2->version = strdup("2.0.0");

    const struct package_metadata *candidates[] = {app, lib1, lib2};
    const struct package_metadata *roots[] = {app};

    struct package_metadata **selected = NULL;
    size_t selected_count = 0;
    char *conflict = NULL;
    sat_solve_result_t r =
        dep_graph_resolve_sat(candidates, 3, roots, 1, 1000, 4, &selected,
                              &selected_count, &conflict);

    assert(r == SAT_SOLVE_SATISFIABLE);
    assert(conflict == NULL);

    bool app_selected = false, lib2_selected = false;
    for (size_t i = 0; i < selected_count; i++)
    {
        if (selected[i] == app)
            app_selected = true;
        if (selected[i] == lib2)
            lib2_selected = true;
    }
    assert(app_selected);
    assert(lib2_selected);

    free(selected);
    package_metadata_free(app);
    package_metadata_free(lib1);
    package_metadata_free(lib2);
    printf("test_sat_public_satisfiable: PASS\n");
}

void
test_sat_public_unsatisfiable(void)
{
    struct package_metadata *a =
        make_pkg("a", NODEPS, (const char *[]){"b"}, 1, NOPROVIDES, NOREPLACES);
    struct package_metadata *b =
        make_pkg("b", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    const struct package_metadata *candidates[] = {a, b};
    const struct package_metadata *roots[] = {a, b};

    struct package_metadata **selected = NULL;
    size_t selected_count = 0;
    char *conflict = NULL;
    sat_solve_result_t r =
        dep_graph_resolve_sat(candidates, 2, roots, 2, 1000, 4, &selected,
                              &selected_count, &conflict);

    assert(r == SAT_SOLVE_UNSATISFIABLE);
    assert(selected == NULL);
    assert(selected_count == 0);
    assert(conflict != NULL);
    assert(strstr(conflict, "a") != NULL);
    assert(strstr(conflict, "b") != NULL);

    free(conflict);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_sat_public_unsatisfiable: PASS\n");
}
