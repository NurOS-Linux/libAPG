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

    test_crc32_known_vectors();
    test_crc32_matches_simple_reference();
    test_md5_known_vectors();
    test_md5_chunking_invariant();
    test_sha256_known_vectors();
    test_sha256_chunking_invariant();

    test_install_data_dir_copies_files();
    test_install_data_dir_missing_data_returns_false();
    test_rollback_install_removes_files();
    test_db_add_get_remove_roundtrip();
    test_run_script_root();

    printf("All 43 tests passed.\n");
    return 0;
}
