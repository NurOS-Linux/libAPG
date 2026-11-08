// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include "tests.h"

int
main(void)
{
    test_linear_resolve();
    test_single_node_resolve();
    test_diamond_resolve();
    test_parallel_resolve();
    test_duplicate_add();
    test_resolve_unknown_root();
    test_missing_transitive_dep();
    test_provides_resolution();
    test_resolve_via_alias();
    test_multiple_providers_prefers_installed();
    test_multiple_providers_falls_back_to_first_when_none_installed();
    test_export_dot();
    test_replaces_resolution();
    test_version_constraint_satisfied();
    test_version_constraint_unsatisfied();
    test_version_exact_match();
    test_epoch_higher_wins();
    test_epoch_zero_implicit();
    test_epoch_equal_falls_through();

    test_self_cycle();
    test_two_node_cycle();
    test_three_way_cycle();
    test_has_cycle_empty();
    test_has_cycle_forest();

    test_conflict_direct_new_vs_installed();
    test_conflict_reverse_installed_vs_new();
    test_conflict_via_new_provides();
    test_conflict_via_installed_provides();
    test_no_conflicts();
    test_breaks_empty_installed();
    test_breaks_unknown_pkg();

    test_policy_unsigned();
    test_policy_no_sig_required();
    test_policy_clear();
    test_policy_missing_dep_rejected_by_default();
    test_prepare_reports_unsatisfied_dep_version();
    test_policy_skip_dependency_check_allows_missing_dep();
    test_policy_remove_blocked_by_dependents_by_default();
    test_remove_with_dependents_removed_together();
    test_remove_orders_dependents_first();
    test_policy_skip_dependents_check_allows_blocked_remove();
    test_dry_run_install_reports_ok_without_side_effects();
    test_dry_run_still_reports_unsigned();
    test_progress_cb_called_per_step();
    test_dry_run_does_not_count_towards_commit_limit();
    test_dry_run_remove_does_not_modify_db();
    test_candidate_resolved_when_needed_and_marked_non_explicit();
    test_candidate_not_planned_when_unneeded();

    test_install_data_dir_copies_files();
    test_install_data_dir_preserves_file_permissions();
    test_install_data_dir_missing_data_returns_false();
    test_rollback_install_removes_files();
    test_db_add_get_remove_roundtrip();
    test_db_readonly_sees_package_files();
    test_db_set_installed_by_hand_skips_journal();
    test_db_add_grows_full_map();
    test_db_open_reports_lock_and_missing_record();
    test_run_script_root();
    test_run_script_reports_exit_status();
    test_install_data_dir_reports_write_error();
    test_parse_package_install_roundtrip();
    test_install_package_in_root_uses_isolated_temp_dirs();
    test_upgrade_preserves_installed_by_hand();
    test_upgrade_removes_dropped_files();
    test_failed_transaction_restores_upgraded_package();
    test_failed_pre_remove_is_reported();

    test_unarchive_preserves_hardlinks();
    test_unarchive_rejects_path_traversal_hardlink();
    test_unarchive_rejects_absolute_hardlink();
    test_unarchive_reports_open_failure();

    test_trans_plan_accessors();
    test_trans_conflict_accessors();
    test_db_verify_issue_accessors();
    test_journal_entry_accessors();
    test_trans_prefer_provider_overrides_installed_default();
    test_trans_default_prefers_installed_provider();
    test_trans_upgrade_in_place_keeps_installed_preference();

    test_sat_public_satisfiable();
    test_sat_public_unsatisfiable();

    test_db_list_empty_and_populated();
    test_db_search_matches_name_and_description();
    test_db_stats_counts_packages_and_files();
    test_db_dependents_direct_and_via_provides();
    test_db_orphans_respect_dependencies_and_flags();
    test_db_set_hold_persists_and_validates();
    test_db_hooks_fire_on_add_and_remove();
    test_db_owner_follows_add_and_remove();
    test_db_dependents_transitive_follows_chains();
    test_db_dependents_transitive_handles_cycles();
    test_db_orphans_transitive_follows_chains();
    test_db_orphans_transitive_handles_cycles_and_provides();

    test_copy_file_preserves_content_and_mode();
    test_copy_file_large_file();
    test_copy_file_reports_failures();
    test_copy_dir_copies_tree_with_modes_and_links();
    test_copy_dir_merges_into_existing_destination();
    test_copy_dir_replaces_mismatched_entry_types();
    test_copy_dir_reports_missing_source();
    test_copy_dir_stops_on_unreadable_file();
    test_concat_dirs_normalizes_separators();
    test_collect_files_lists_relative_paths();
    test_remove_dir_recursive_does_not_follow_links();

    test_json_parses_all_fields();
    test_json_roundtrip_is_stable();
    test_json_rejects_malformed_input();
    test_json_respects_length_argument();
    test_json_ignores_wrongly_typed_scalars();
    test_json_string_lists_never_contain_null_items();
    test_json_dependencies_never_contain_null_names();
    test_json_defaults_when_serializing_empty_package();
    test_json_unicode_and_escapes_survive_roundtrip();
    test_json_metadata_from_file();
    test_dep_constraint_string_roundtrip();

    test_keyring_load_handles_directory_contents();
    test_keyring_verify_accepts_only_valid_signatures();
    test_keyring_verify_large_and_empty_packages();
    test_keyring_add_key_requires_endorsement();
    test_keyring_add_key_with_any_filename_is_loadable();
    test_sign_file_and_verify_roundtrip();
    test_sign_with_key_roundtrip_and_failures();

    test_journal_read_without_journal_is_empty();
    test_journal_write_read_roundtrip();
    test_journal_preserves_write_order();
    test_journal_null_name_and_version_roundtrip();
    test_journal_fields_with_separator_survive();
    test_journal_long_names_are_not_silently_corrupted();
    test_journal_db_operations_are_recorded();
    test_journal_removing_missing_package_is_not_recorded_as_ok();

    test_trans_held_upgrade_is_reported();
    test_trans_held_remove_is_reported_and_others_planned();
    test_trans_released_hold_allows_operation();
    test_trans_many_held_packages_are_all_reported();
    test_trans_file_conflict_is_reported();
    test_trans_many_file_conflicts_are_all_reported();
    test_trans_own_files_do_not_conflict();
    test_trans_removed_owner_frees_its_files();
    test_trans_error_priority_held_before_file_conflict();
    test_trans_accessors_handle_bad_arguments();
    test_trans_commit_state_machine();
    test_trans_remove_of_unknown_package_does_not_corrupt_db();

    test_run_script_strict_matches_run_script_when_isolated();
    test_run_script_strict_refuses_when_isolation_is_denied();
    test_trans_require_sandbox_controls_script_isolation();

    test_trans_estimate_space_for_installs();
    test_trans_estimate_space_counts_upgrade_backups();
    test_trans_estimate_space_edge_cases();

    test_db_check_reports_nothing_for_consistent_database();
    test_db_check_reports_ownership_inconsistencies();
    test_db_check_reports_record_inconsistencies();

    test_verify_against_archive_accepts_clean_install();
    test_verify_against_archive_reports_differences();
    test_verify_against_archive_rejects_bad_requests();

    test_package_read_metadata_reads_archive();
    test_package_read_metadata_reports_failures();
    test_package_list_files_matches_install();
    test_package_list_files_edge_cases();

    test_trans_step_required_by_follows_dependency_chain();
    test_trans_step_required_by_is_unset_for_other_steps();

    test_trans_commit_hooks_run_around_commit();
    test_trans_commit_hooks_report_failures_and_skip_dry_runs();

    test_audit_read_since_filters_by_timestamp();
    test_audit_read_for_package_matches_exact_names();
    test_audit_filters_on_database_without_journal();

    printf("All 163 tests passed.\n");
    return 0;
}
