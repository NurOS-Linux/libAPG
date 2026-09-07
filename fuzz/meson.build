if get_option('fuzzing').enabled()
  fuzzer_c_args = ['-fsanitize=fuzzer,address,undefined']
  fuzzer_link_args = fuzzer_c_args + ['-lm']

  fuzz_package_json = executable('fuzz-package-json',
    'fuzz_package_json.c',
    dependencies: libapg_dep,
    c_args: fuzzer_c_args,
    link_args: fuzzer_link_args,
    build_by_default: false,
  )

  fuzz_dep_constraint = executable('fuzz-dep-constraint',
    'fuzz_dep_constraint.c',
    dependencies: libapg_dep,
    c_args: fuzzer_c_args,
    link_args: fuzzer_link_args,
    build_by_default: false,
  )

  fuzz_sat_model = executable('fuzz-sat-model',
    'fuzz_sat_model.c',
    '../src/graph/candidates.c',
    '../src/graph/sat_model.c',
    '../src/graph/sat_solve.c',
    '../src/hashmap.c',
    dependencies: [libapg_dep, threads_dep],
    c_args: fuzzer_c_args,
    link_args: fuzzer_link_args,
    build_by_default: false,
  )

  fuzz_candidate_set = executable('fuzz-candidate-set',
    'fuzz_candidate_set.c',
    '../src/graph/candidates.c',
    '../src/hashmap.c',
    dependencies: libapg_dep,
    c_args: fuzzer_c_args,
    link_args: fuzzer_link_args,
    build_by_default: false,
  )

  fuzz_sat_model_vars = executable('fuzz-sat-model-vars',
    'fuzz_sat_model_vars.c',
    '../src/graph/candidates.c',
    '../src/graph/sat_model.c',
    '../src/graph/sat_solve.c',
    '../src/hashmap.c',
    dependencies: [libapg_dep, threads_dep],
    c_args: fuzzer_c_args,
    link_args: fuzzer_link_args,
    build_by_default: false,
  )

  fuzz_db = executable('fuzz-db',
    'fuzz_db.c',
    dependencies: libapg_dep,
    c_args: fuzzer_c_args,
    link_args: fuzzer_link_args,
    build_by_default: false,
  )

  fuzz_install_data_dir = executable('fuzz-install-data-dir',
    'fuzz_install_data_dir.c',
    dependencies: libapg_dep,
    c_args: fuzzer_c_args,
    link_args: fuzzer_link_args,
    build_by_default: false,
  )

  fuzz_transaction_prepare = executable('fuzz-transaction-prepare',
    'fuzz_transaction_prepare.c',
    dependencies: [libapg_dep, threads_dep],
    c_args: fuzzer_c_args,
    link_args: fuzzer_link_args,
    build_by_default: false,
  )
endif
