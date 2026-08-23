// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "helpers.h"
#include "../../src/graph/candidates_priv.h"
#include "../../src/graph/sat_model_priv.h"
#include "../../src/graph/sat_solve_priv.h"

void
test_sat_empty_model_satisfiable(void)
{
    struct sat_model m;
    assert(sat_model_init(&m));

    int *assignment = NULL;
    enum sat_result r = sat_solve(&m, 100, &assignment, NULL);
    assert(r == SAT_RESULT_SATISFIABLE);
    assert(assignment == NULL);

    sat_model_free(&m);
    printf("test_sat_empty_model_satisfiable: PASS\n");
}

void
test_sat_dependency_selects_satisfying_candidate(void)
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

    struct candidate_set cs;
    assert(candidate_set_init(&cs));
    assert(candidate_set_add_package(&cs, app));
    assert(candidate_set_add_package(&cs, lib1));
    assert(candidate_set_add_package(&cs, lib2));

    const struct package_metadata *pkgs[] = {app, lib1, lib2};

    struct sat_model m;
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, 3));
    assert(sat_model_force(&m, app));

    int app_var = sat_model_var(&m, app);
    int lib2_var = sat_model_var(&m, lib2);

    int *assignment = NULL;
    enum sat_result r = sat_solve(&m, 1000, &assignment, NULL);
    assert(r == SAT_RESULT_SATISFIABLE);
    assert(assignment != NULL);
    assert(assignment[app_var] == 1);
    assert(assignment[lib2_var] == 1);

    free(assignment);
    sat_model_free(&m);
    candidate_set_free(&cs);
    package_metadata_free(app);
    package_metadata_free(lib1);
    package_metadata_free(lib2);
    printf("test_sat_dependency_selects_satisfying_candidate: PASS\n");
}

void
test_sat_dependency_unsatisfiable_when_version_excludes_only_candidate(void)
{
    struct package_metadata *app =
        make_pkg("app", (const char *[]){"lib>=2.0.0"}, 1, NOCONFLICTS,
                 NOPROVIDES, NOREPLACES);
    struct package_metadata *lib1 =
        make_pkg("lib", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
    free(lib1->version);
    lib1->version = strdup("1.0.0");

    struct candidate_set cs;
    assert(candidate_set_init(&cs));
    assert(candidate_set_add_package(&cs, app));
    assert(candidate_set_add_package(&cs, lib1));

    const struct package_metadata *pkgs[] = {app, lib1};

    struct sat_model m;
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, 2));
    assert(sat_model_force(&m, app));

    int *assignment = NULL;
    char *conflict = NULL;
    enum sat_result r = sat_solve(&m, 1000, &assignment, &conflict);
    assert(r == SAT_RESULT_UNSATISFIABLE);
    assert(assignment == NULL);
    assert(conflict != NULL);
    assert(strstr(conflict, "app") != NULL);
    assert(strstr(conflict, "lib") != NULL);
    free(conflict);

    sat_model_free(&m);
    candidate_set_free(&cs);
    package_metadata_free(app);
    package_metadata_free(lib1);
    printf("test_sat_dependency_unsatisfiable_when_version_excludes_only_"
           "candidate: "
           "PASS\n");
}

void
test_sat_multi_provider_at_least_one(void)
{
    struct package_metadata *app =
        make_pkg("app", (const char *[]){"mailer"}, 1, NOCONFLICTS, NOPROVIDES,
                 NOREPLACES);
    struct package_metadata *mailer_a =
        make_pkg("mailer-a", NODEPS, NOCONFLICTS, (const char *[]){"mailer"}, 1,
                 NOREPLACES);
    struct package_metadata *mailer_b =
        make_pkg("mailer-b", NODEPS, NOCONFLICTS, (const char *[]){"mailer"}, 1,
                 NOREPLACES);

    struct candidate_set cs;
    assert(candidate_set_init(&cs));
    assert(candidate_set_add_package(&cs, app));
    assert(candidate_set_add_package(&cs, mailer_a));
    assert(candidate_set_add_package(&cs, mailer_b));

    const struct package_metadata *pkgs[] = {app, mailer_a, mailer_b};

    struct sat_model m;
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, 3));
    assert(sat_model_force(&m, app));

    int *assignment = NULL;
    enum sat_result r = sat_solve(&m, 1000, &assignment, NULL);
    assert(r == SAT_RESULT_SATISFIABLE);
    assert(assignment[sat_model_var(&m, app)] == 1);
    bool a_selected = assignment[sat_model_var(&m, mailer_a)] == 1;
    bool b_selected = assignment[sat_model_var(&m, mailer_b)] == 1;
    assert(a_selected || b_selected);

    free(assignment);
    sat_model_free(&m);
    candidate_set_free(&cs);
    package_metadata_free(app);
    package_metadata_free(mailer_a);
    package_metadata_free(mailer_b);
    printf("test_sat_multi_provider_at_least_one: PASS\n");
}

void
test_sat_conflict_forces_unsat_when_both_pinned(void)
{
    struct package_metadata *a =
        make_pkg("a", NODEPS, (const char *[]){"b"}, 1, NOPROVIDES, NOREPLACES);
    struct package_metadata *b =
        make_pkg("b", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct candidate_set cs;
    assert(candidate_set_init(&cs));
    assert(candidate_set_add_package(&cs, a));
    assert(candidate_set_add_package(&cs, b));

    const struct package_metadata *pkgs[] = {a, b};

    struct sat_model m;
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, 2));
    assert(sat_model_force(&m, a));
    assert(sat_model_force(&m, b));

    int *assignment = NULL;
    char *conflict = NULL;
    enum sat_result r = sat_solve(&m, 1000, &assignment, &conflict);
    assert(r == SAT_RESULT_UNSATISFIABLE);
    assert(assignment == NULL);
    assert(conflict != NULL);
    assert(strstr(conflict, "a") != NULL);
    assert(strstr(conflict, "b") != NULL);
    free(conflict);

    sat_model_free(&m);
    candidate_set_free(&cs);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_sat_conflict_forces_unsat_when_both_pinned: PASS\n");
}

void
test_sat_no_conflict_clause_when_not_pinned(void)
{
    struct package_metadata *a =
        make_pkg("a", NODEPS, (const char *[]){"b"}, 1, NOPROVIDES, NOREPLACES);
    struct package_metadata *b =
        make_pkg("b", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct candidate_set cs;
    assert(candidate_set_init(&cs));
    assert(candidate_set_add_package(&cs, a));
    assert(candidate_set_add_package(&cs, b));

    const struct package_metadata *pkgs[] = {a, b};

    struct sat_model m;
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, 2));

    int *assignment = NULL;
    enum sat_result r = sat_solve(&m, 1000, &assignment, NULL);
    assert(r == SAT_RESULT_SATISFIABLE);

    free(assignment);
    sat_model_free(&m);
    candidate_set_free(&cs);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_sat_no_conflict_clause_when_not_pinned: PASS\n");
}

void
test_sat_budget_exceeded_on_many_free_vars(void)
{
    enum
    {
        PKG_COUNT = 20
    };
    struct package_metadata *pkgs_meta[PKG_COUNT];
    const struct package_metadata *pkgs[PKG_COUNT];
    char name[32];

    struct candidate_set cs;
    assert(candidate_set_init(&cs));

    for (int i = 0; i < PKG_COUNT; i++)
    {
        snprintf(name, sizeof(name), "free-%d", i);
        pkgs_meta[i] =
            make_pkg(name, NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);
        pkgs[i] = pkgs_meta[i];
        assert(candidate_set_add_package(&cs, pkgs_meta[i]));
    }

    struct sat_model m;
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, PKG_COUNT));

    int *assignment = NULL;
    enum sat_result r = sat_solve(&m, 2, &assignment, NULL);
    assert(r == SAT_RESULT_BUDGET_EXCEEDED);
    assert(assignment == NULL);

    sat_model_free(&m);
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, PKG_COUNT));

    r = sat_solve(&m, PKG_COUNT, &assignment, NULL);
    assert(r == SAT_RESULT_SATISFIABLE);
    assert(assignment != NULL);

    free(assignment);
    sat_model_free(&m);
    candidate_set_free(&cs);
    for (int i = 0; i < PKG_COUNT; i++)
        package_metadata_free(pkgs_meta[i]);
    printf("test_sat_budget_exceeded_on_many_free_vars: PASS\n");
}

void
test_sat_solve_parallel_matches_sequential(void)
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

    struct candidate_set cs;
    assert(candidate_set_init(&cs));
    assert(candidate_set_add_package(&cs, app));
    assert(candidate_set_add_package(&cs, lib1));
    assert(candidate_set_add_package(&cs, lib2));

    const struct package_metadata *pkgs[] = {app, lib1, lib2};

    struct sat_model m;
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, 3));
    assert(sat_model_force(&m, app));

    int app_var = sat_model_var(&m, app);
    int lib2_var = sat_model_var(&m, lib2);

    int *assignment = NULL;
    char *conflict = NULL;
    enum sat_result r = sat_solve_parallel(&m, 1000, 4, &assignment, &conflict);
    assert(r == SAT_RESULT_SATISFIABLE);
    assert(assignment != NULL);
    assert(assignment[app_var] == 1);
    assert(assignment[lib2_var] == 1);
    assert(conflict == NULL);

    free(assignment);
    sat_model_free(&m);
    candidate_set_free(&cs);
    package_metadata_free(app);
    package_metadata_free(lib1);
    package_metadata_free(lib2);
    printf("test_sat_solve_parallel_matches_sequential: PASS\n");
}

void
test_sat_solve_parallel_reports_unsat(void)
{
    struct package_metadata *a =
        make_pkg("a", NODEPS, (const char *[]){"b"}, 1, NOPROVIDES, NOREPLACES);
    struct package_metadata *b =
        make_pkg("b", NODEPS, NOCONFLICTS, NOPROVIDES, NOREPLACES);

    struct candidate_set cs;
    assert(candidate_set_init(&cs));
    assert(candidate_set_add_package(&cs, a));
    assert(candidate_set_add_package(&cs, b));

    const struct package_metadata *pkgs[] = {a, b};

    struct sat_model m;
    assert(sat_model_init(&m));
    assert(sat_model_build(&m, &cs, pkgs, 2));
    assert(sat_model_force(&m, a));
    assert(sat_model_force(&m, b));

    int *assignment = NULL;
    char *conflict = NULL;
    enum sat_result r = sat_solve_parallel(&m, 1000, 4, &assignment, &conflict);
    assert(r == SAT_RESULT_UNSATISFIABLE);
    assert(assignment == NULL);
    assert(conflict != NULL);
    free(conflict);

    sat_model_free(&m);
    candidate_set_free(&cs);
    package_metadata_free(a);
    package_metadata_free(b);
    printf("test_sat_solve_parallel_reports_unsat: PASS\n");
}
