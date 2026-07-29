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
endif
