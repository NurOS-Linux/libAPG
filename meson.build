# NurOS Ruzen42 2025
project('libapg', 'c',
  version: '1.1.0'
)

libapg_sources = files(
  'src/package.c',
  'src/util.c',
  'src/archive.c',
  'src/crc32.c',
  'src/db.c',
  'src/json.c',
  'src/repo/repo_fetch.c'
)

add_project_arguments('-D_GNU_SOURCE', language: 'c')

libapg_inc = include_directories('include')

libarchive_dep = dependency('libarchive', required: false)
lmdb_dep = dependency('lmdb', required: false)
libiron_dep = dependency('libiron', required: false)
lmdb_dep = dependency('lmdb', required: false)
sodium_dep = dependency('libsodium', required: false)

cc = meson.get_compiler('c')

libyyjson_dep = dependency('yyjson', required: false)

cc = meson.get_compiler('c')

if not libiron_dep.found()
  libiron_dep = cc.find_library('iron', required: true)
endif

if not libarchive_dep.found()
  libarchive_dep = cc.find_library('archive', required: true)
endif

if not lmdb_dep.found()
  lmdb_dep = cc.find_library('lmdb', required: true)
endif

if not libyyjson_dep.found()
  libyyjson = cc.find_library('yyjson', required: true)
endif

if not libyyjson_dep.found()
  sodium_dep = cc.find_library('sodium', required: true)
endif

libapg = library('apg', libapg_sources,
  include_directories: libapg_inc,
  dependencies: [
    libarchive_dep,
    libiron_dep,
    lmdb_dep,
    libyyjson_dep,
    sodium_dep
  ],
  install: true,
)

libapg_dep = declare_dependency(
  link_with: libapg,
  include_directories: libapg_inc,
)
