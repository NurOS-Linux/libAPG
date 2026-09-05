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
endif
