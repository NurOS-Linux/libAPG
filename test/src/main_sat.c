// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>

void test_sat_empty_model_satisfiable(void);
void test_sat_dependency_selects_satisfying_candidate(void);
void
test_sat_dependency_unsatisfiable_when_version_excludes_only_candidate(void);
void test_sat_multi_provider_at_least_one(void);
void test_sat_conflict_forces_unsat_when_both_pinned(void);
void test_sat_no_conflict_clause_when_not_pinned(void);
void test_sat_budget_exceeded_on_many_free_vars(void);

int
main(void)
{
    test_sat_empty_model_satisfiable();
    test_sat_dependency_selects_satisfying_candidate();
    test_sat_dependency_unsatisfiable_when_version_excludes_only_candidate();
    test_sat_multi_provider_at_least_one();
    test_sat_conflict_forces_unsat_when_both_pinned();
    test_sat_no_conflict_clause_when_not_pinned();
    test_sat_budget_exceeded_on_many_free_vars();

    printf("All 7 SAT tests passed.\n");
    return 0;
}
