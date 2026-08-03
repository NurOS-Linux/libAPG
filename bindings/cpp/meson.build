if get_option('cxx_bindings').enabled()
  add_languages('cpp', required: true, native: false)

  cpp_bindings_headers = files(
    'include/apg++/apg.hpp',
    'include/apg++/archive.hpp',
    'include/apg++/audit.hpp',
    'include/apg++/config.hpp',
    'include/apg++/copy.hpp',
    'include/apg++/db.hpp',
    'include/apg++/detail.hpp',
    'include/apg++/error.hpp',
    'include/apg++/graph.hpp',
    'include/apg++/install.hpp',
    'include/apg++/journal.hpp',
    'include/apg++/keyring.hpp',
    'include/apg++/package.hpp',
    'include/apg++/scripts.hpp',
    'include/apg++/sign.hpp',
    'include/apg++/transaction.hpp',
    'include/apg++/version.hpp',
  )

  install_headers(cpp_bindings_headers, subdir: 'apg++')

  cpp_bindings_inc = include_directories('include')

  cpp_bindings_test = executable('apgxx-test',
    'test/test_bindings.cpp',
    include_directories: cpp_bindings_inc,
    dependencies: libapg_dep,
    override_options: ['cpp_std=c++17'],
    build_by_default: false,
  )

  test('apgxx-bindings', cpp_bindings_test)
endif
