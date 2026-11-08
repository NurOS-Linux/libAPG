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
void test_multiple_providers_prefers_installed(void);
void test_multiple_providers_falls_back_to_first_when_none_installed(void);
void test_export_dot(void);
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
void test_policy_missing_dep_rejected_by_default(void);
void test_prepare_reports_unsatisfied_dep_version(void);
void test_policy_skip_dependency_check_allows_missing_dep(void);
void test_policy_remove_blocked_by_dependents_by_default(void);
void test_remove_with_dependents_removed_together(void);
void test_remove_orders_dependents_first(void);
void test_policy_skip_dependents_check_allows_blocked_remove(void);
void test_dry_run_install_reports_ok_without_side_effects(void);
void test_dry_run_still_reports_unsigned(void);
void test_progress_cb_called_per_step(void);
void test_dry_run_does_not_count_towards_commit_limit(void);
void test_dry_run_remove_does_not_modify_db(void);
void test_candidate_resolved_when_needed_and_marked_non_explicit(void);
void test_candidate_not_planned_when_unneeded(void);

void test_install_data_dir_copies_files(void);
void test_install_data_dir_preserves_file_permissions(void);
void test_install_data_dir_missing_data_returns_false(void);
void test_rollback_install_removes_files(void);
void test_db_add_get_remove_roundtrip(void);
void test_db_readonly_sees_package_files(void);
void test_db_set_installed_by_hand_skips_journal(void);
void test_db_add_grows_full_map(void);
void test_db_open_reports_lock_and_missing_record(void);
void test_run_script_root(void);
void test_run_script_reports_exit_status(void);
void test_install_data_dir_reports_write_error(void);
void test_parse_package_install_roundtrip(void);
void test_install_package_in_root_uses_isolated_temp_dirs(void);
void test_upgrade_preserves_installed_by_hand(void);
void test_upgrade_removes_dropped_files(void);
void test_failed_transaction_restores_upgraded_package(void);
void test_failed_pre_remove_is_reported(void);

void test_unarchive_preserves_hardlinks(void);
void test_unarchive_rejects_path_traversal_hardlink(void);
void test_unarchive_rejects_absolute_hardlink(void);
void test_unarchive_reports_open_failure(void);

void test_trans_plan_accessors(void);
void test_trans_conflict_accessors(void);
void test_db_verify_issue_accessors(void);
void test_journal_entry_accessors(void);
void test_trans_prefer_provider_overrides_installed_default(void);
void test_trans_default_prefers_installed_provider(void);
void test_trans_upgrade_in_place_keeps_installed_preference(void);

void test_sat_public_satisfiable(void);
void test_sat_public_unsatisfiable(void);

void test_db_list_empty_and_populated(void);
void test_db_search_matches_name_and_description(void);
void test_db_stats_counts_packages_and_files(void);
void test_db_dependents_direct_and_via_provides(void);
void test_db_orphans_respect_dependencies_and_flags(void);
void test_db_set_hold_persists_and_validates(void);
void test_db_hooks_fire_on_add_and_remove(void);
void test_db_owner_follows_add_and_remove(void);
void test_db_dependents_transitive_follows_chains(void);
void test_db_dependents_transitive_handles_cycles(void);
void test_db_orphans_transitive_follows_chains(void);
void test_db_orphans_transitive_handles_cycles_and_provides(void);

void test_copy_file_preserves_content_and_mode(void);
void test_copy_file_large_file(void);
void test_copy_file_reports_failures(void);
void test_copy_dir_copies_tree_with_modes_and_links(void);
void test_copy_dir_merges_into_existing_destination(void);
void test_copy_dir_replaces_mismatched_entry_types(void);
void test_copy_dir_reports_missing_source(void);
void test_copy_dir_stops_on_unreadable_file(void);
void test_concat_dirs_normalizes_separators(void);
void test_collect_files_lists_relative_paths(void);
void test_remove_dir_recursive_does_not_follow_links(void);

void test_json_parses_all_fields(void);
void test_json_roundtrip_is_stable(void);
void test_json_rejects_malformed_input(void);
void test_json_respects_length_argument(void);
void test_json_ignores_wrongly_typed_scalars(void);
void test_json_string_lists_never_contain_null_items(void);
void test_json_dependencies_never_contain_null_names(void);
void test_json_defaults_when_serializing_empty_package(void);
void test_json_unicode_and_escapes_survive_roundtrip(void);
void test_json_metadata_from_file(void);
void test_dep_constraint_string_roundtrip(void);

void test_keyring_load_handles_directory_contents(void);
void test_keyring_verify_accepts_only_valid_signatures(void);
void test_keyring_verify_large_and_empty_packages(void);
void test_keyring_add_key_requires_endorsement(void);
void test_keyring_add_key_with_any_filename_is_loadable(void);
void test_sign_file_and_verify_roundtrip(void);
void test_sign_with_key_roundtrip_and_failures(void);

void test_journal_read_without_journal_is_empty(void);
void test_journal_write_read_roundtrip(void);
void test_journal_preserves_write_order(void);
void test_journal_null_name_and_version_roundtrip(void);
void test_journal_fields_with_separator_survive(void);
void test_journal_long_names_are_not_silently_corrupted(void);
void test_journal_db_operations_are_recorded(void);
void test_journal_removing_missing_package_is_not_recorded_as_ok(void);

void test_trans_held_upgrade_is_reported(void);
void test_trans_held_remove_is_reported_and_others_planned(void);
void test_trans_released_hold_allows_operation(void);
void test_trans_many_held_packages_are_all_reported(void);
void test_trans_file_conflict_is_reported(void);
void test_trans_many_file_conflicts_are_all_reported(void);
void test_trans_own_files_do_not_conflict(void);
void test_trans_removed_owner_frees_its_files(void);
void test_trans_error_priority_held_before_file_conflict(void);
void test_trans_accessors_handle_bad_arguments(void);
void test_trans_commit_state_machine(void);
void test_trans_remove_of_unknown_package_does_not_corrupt_db(void);

void test_run_script_strict_matches_run_script_when_isolated(void);
void test_run_script_strict_refuses_when_isolation_is_denied(void);
void test_trans_require_sandbox_controls_script_isolation(void);

void test_trans_estimate_space_for_installs(void);
void test_trans_estimate_space_counts_upgrade_backups(void);
void test_trans_estimate_space_edge_cases(void);

void test_db_check_reports_nothing_for_consistent_database(void);
void test_db_check_reports_ownership_inconsistencies(void);
void test_db_check_reports_record_inconsistencies(void);

void test_verify_against_archive_accepts_clean_install(void);
void test_verify_against_archive_reports_differences(void);
void test_verify_against_archive_rejects_bad_requests(void);

void test_package_read_metadata_reads_archive(void);
void test_package_read_metadata_reports_failures(void);
void test_package_list_files_matches_install(void);
void test_package_list_files_edge_cases(void);

void test_trans_step_required_by_follows_dependency_chain(void);
void test_trans_step_required_by_is_unset_for_other_steps(void);

void test_trans_commit_hooks_run_around_commit(void);
void test_trans_commit_hooks_report_failures_and_skip_dry_runs(void);
