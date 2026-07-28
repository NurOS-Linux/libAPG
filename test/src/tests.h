// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

void test_linear_resolve(void);
void test_single_node_resolve(void);
void test_diamond_resolve(void);
void test_parallel_resolve(void);
void test_duplicate_add(void);
void test_resolve_unknown_root(void);
void test_missing_transitive_dep(void);
void test_provides_resolution(void);
void test_resolve_via_alias(void);
void test_replaces_resolution(void);
void test_version_constraint_satisfied(void);
void test_version_constraint_unsatisfied(void);
void test_version_exact_match(void);
void test_epoch_higher_wins(void);
void test_epoch_zero_implicit(void);
void test_epoch_equal_falls_through(void);

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

void test_policy_unsigned(void);
void test_policy_no_sig_required(void);
void test_policy_clear(void);

void test_crc32_known_vectors(void);
void test_crc32_matches_simple_reference(void);
void test_md5_known_vectors(void);
void test_md5_chunking_invariant(void);
void test_sha256_known_vectors(void);
void test_sha256_chunking_invariant(void);

void test_install_data_dir_copies_files(void);
void test_install_data_dir_missing_data_returns_false(void);
void test_rollback_install_removes_files(void);
void test_db_add_get_remove_roundtrip(void);
void test_run_script_root(void);
void test_parse_package_install_roundtrip(void);
void test_install_package_in_root_uses_isolated_temp_dirs(void);

void test_verify_checksums_sha256_valid_passes(void);
void test_verify_checksums_sha256_tampered_fails(void);
void test_verify_checksums_crc32_fallback_passes(void);
void test_verify_checksums_md5_fallback_passes(void);
void test_verify_checksums_missing_sums_file_fails(void);
void test_verify_checksums_fuzz_malformed_input_no_crash(void);

void test_trans_plan_accessors(void);
void test_trans_conflict_accessors(void);
void test_db_verify_issue_accessors(void);
void test_journal_entry_accessors(void);
