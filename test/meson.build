apg_test = executable('apg-test',
  'src/main.c',
  'src/test_resolve.c',
  'src/test_cycle.c',
  'src/test_conflict.c',
  'src/test_policy.c',
  'src/test_install.c',
  'src/test_accessors.c',
  dependencies: libapg_dep,
  build_by_default: false,
)

test('apg-test', apg_test)

sat_test = executable('sat-test',
  'src/main_sat.c',
  'src/test_sat.c',
  '../src/graph/candidates.c',
  '../src/graph/sat_model.c',
  '../src/graph/sat_solve.c',
  '../src/hashmap.c',
  dependencies: libapg_dep,
  build_by_default: false,
)

test('sat-test', sat_test)
