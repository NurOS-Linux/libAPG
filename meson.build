project('libapg', 'c',
  version: '2.1.0',
  default_options: [
    'libarchive:tests=disabled',
    'libarchive:progs=[]',
    'libarchive:openssl=disabled',
    'libarchive:xml2=disabled',
    'libarchive:expat=disabled',
    'libarchive:cng=disabled',
    'libarchive:iconv=disabled',
  ]
)

libapg_sources = files(
  'src/package.c',
  'src/util.c',
  'src/archive.c',
  'src/install/copy.c',
  'src/install/install.c',
  'src/install/rollback.c',
  'src/install/scripts.c',
  'src/db/db.c',
  'src/db/write.c',
  'src/db/read.c',
  'src/db/owner.c',
  'src/db/journal.c',
  'src/db/audit.c',
  'src/db/dependents.c',
  'src/db/orphans.c',
  'src/db/str_vec.c',
  'src/hashmap.c',
  'src/db/search.c',
  'src/db/stats.c',
  'src/db/verify.c',
  'src/json.c',
  'src/graph/graph.c',
  'src/graph/resolve.c',
  'src/graph/conflict.c',
  'src/graph/dot.c',
  'src/graph/candidates.c',
  'src/graph/sat_model.c',
  'src/graph/sat_solve.c',
  'src/version/version.c',
  'src/transaction/transaction.c',
  'src/transaction/prepare.c',
  'src/transaction/commit.c',
)

if host_machine.system() == 'linux'
  add_project_arguments('-D_GNU_SOURCE', language: 'c')
endif

keyring_dir = get_option('keyring_dir')
keys_dir = get_option('keys_dir')
if host_machine.system() == 'freebsd'
  if keyring_dir == '/etc/apg/trusted.d'
    keyring_dir = '/usr/local/etc/apg/trusted.d'
  endif
  if keys_dir == '/etc/apg/keys'
    keys_dir = '/usr/local/etc/apg/keys'
  endif
endif

add_project_arguments(
  '-DAPG_KEYRING_DIR="' + keyring_dir + '"',
  '-DAPG_KEYS_DIR="' + keys_dir + '"',
  '-DAPG_TMP_DIR="' + get_option('tmp_dir') + '"',
  '-DAPG_DB_MAPSIZE=' + get_option('db_mapsize').to_string(),
  language: 'c',
)

libapg_inc = include_directories('include')

cc = meson.get_compiler('c')

if get_option('fuzzing').enabled()
  if cc.get_id() != 'clang'
    error('fuzzing requires clang (libFuzzer)')
  endif
  add_project_arguments('-fsanitize=fuzzer-no-link,address,undefined',
                        language: 'c')
  add_project_link_arguments('-fsanitize=address,undefined', '-shared-libsan',
                             language: 'c')
endif

threads_dep = dependency('threads')

libarchive_dep = dependency('libarchive', fallback: ['libarchive', 'libarchive_dep'])
lmdb_dep = dependency('lmdb', fallback: ['lmdb', 'lmdb_dep'])
libyyjson_dep = dependency('yyjson', fallback: ['yyjson', 'yyjson_dep'])

sodium_dep = dependency('libsodium', required: false)
if not sodium_dep.found()
  sodium_dep = cc.find_library('sodium', required: true)
endif

libapg_sources += files(
  'src/sign/sodium/sodium.c',
  'src/sign/sodium/keyring.c',
)
sign_dep = [sodium_dep]

seccomp_dep = dependency('libseccomp', required: false)
if seccomp_dep.found()
  add_project_arguments('-DAPG_HAVE_SECCOMP=1', language: 'c')
endif

libapg = library('apg', libapg_sources,
  version: meson.project_version(),
  soversion: meson.project_version(),
  gnu_symbol_visibility: 'hidden',
  include_directories: libapg_inc,
  dependencies: [
    libarchive_dep,
    lmdb_dep,
    libyyjson_dep,
    threads_dep,
  ] + sign_dep + (seccomp_dep.found() ? [seccomp_dep] : []),
  install: true,
)

libapg_dep = declare_dependency(
  link_with: libapg,
  include_directories: libapg_inc,
  # journal.h (a public header) includes <lmdb.h> directly; consumers built
  # via this in-tree dependency object need lmdb's include path too. External
  # consumers going through pkg-config already get this via Requires.private.
  dependencies: [lmdb_dep],
)

install_headers(
  'include/apg/archive.h',
  'include/apg/audit.h',
  'include/apg/config.h',
  'include/apg/copy.h',
  'include/apg/db.h',
  'include/apg/export.h',
  'include/apg/graph.h',
  'include/apg/install.h',
  'include/apg/journal.h',
  'include/apg/json.h',
  'include/apg/keyring.h',
  'include/apg/package.h',
  'include/apg/scripts.h',
  'include/apg/sign.h',
  'include/apg/transaction.h',
  'include/apg/version.h',
  subdir: 'apg',
)

pkg = import('pkgconfig')
pkg.generate(libapg,
  name: 'libapg',
  filebase: 'libapg',
  description: 'Package management library for NurOS',
  url: 'https://git.nuros.org/core/libapg',
)

install_data('cmake/libapg-config.cmake',
  install_dir: get_option('libdir') / 'cmake' / 'libapg',
)

doxygen = find_program('doxygen', required: false)
if doxygen.found()
  custom_target('doxygen',
    input: 'Doxyfile',
    output: 'xml',
    command: [doxygen, '@INPUT@'],
    build_by_default: false,
  )
endif

subdir('test')
subdir('fuzz')
subdir('bindings/cpp')
subdir('bindings/python')
