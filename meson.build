# NurOS Ruzen42 2025
project('libapg', 'c',
  version: '1.1.0',
  default_options: ['c_std=c11']
)

libapg_sources = files(
  'src/package.c',
  'src/util.c',
  'src/archive.c',
  'src/crc32.c',
  'src/db.c',
  'src/repo/repo_fetch.c'
)

libapg_inc = include_directories('include')

libarchive_dep = dependency('libarchive', required: false)
lmdb_dep = dependency('lmdb', required: false)

libiron_dep = dependency('libiron', required: false)
lmdb_dep = dependency('lmdb', required: false)

cc = meson.get_compiler('c')

libcJSON_dep = dependency('cJSON', required: false)

cc = meson.get_compiler('c')

if not libiron_dep.found()
  libiron_dep = cc.find_library('iron', required: true)
endif

if not libarchive_dep.found()
  libarchive_dep = cc.find_library('archive', required: true)
endif

<<<<<<< HEAD
if not lmdb_dep.found()
  lmdb_dep = cc.find_library('lmdb', required: true)
=======
if not libcJSON_dep.found()
  libarchive_dep = cc.find_library('cJSON', required: true)
>>>>>>> e6395cd (feat(add db realization))
endif

libapg = library('apg', libapg_sources,
  include_directories: libapg_inc,
  dependencies: [
    libarchive_dep,
    libiron_dep,
<<<<<<< HEAD
    lmdb_dep
=======
    libcJSON_dep
>>>>>>> 43585c5 (feat(add db realization))
  ],
  install: true,
)

libapg_dep = declare_dependency(
  link_with: libapg,
  include_directories: libapg_inc,
)
