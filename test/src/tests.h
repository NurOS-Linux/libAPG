// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

void test_linear_resolve(void);
void test_single_node_resolve(void);
void test_diamond_resolve(void);
void test_duplicate_add(void);
void test_resolve_unknown_root(void);
void test_missing_transitive_dep(void);
void test_provides_resolution(void);
void test_resolve_via_alias(void);
void test_replaces_resolution(void);
void test_version_constraint_satisfied(void);
void test_version_constraint_unsatisfied(void);
void test_version_exact_match(void);

void test_self_cycle(void);
void test_two_node_cycle(void);
void test_three_way_cycle(void);
void test_has_cycle_empty(void);
void test_has_cycle_forest(void);

void test_conflict_direct_new_vs_installed(void);
void test_conflict_reverse_installed_vs_new(void);
void test_conflict_via_new_provides(void);
void test_conflict_via_installed_provides(void);
void test_no_conflicts(void);
void test_breaks_empty_installed(void);
void test_breaks_unknown_pkg(void);
