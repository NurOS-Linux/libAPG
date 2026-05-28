apg_test = executable('apg-test',
  'src/main.c',
  'src/test_resolve.c',
  'src/test_cycle.c',
  'src/test_conflict.c',
  dependencies: libapg_dep,
  build_by_default: false,
)

test('apg-test', apg_test)
