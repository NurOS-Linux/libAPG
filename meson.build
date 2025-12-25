# NurOS Ruzen42 2025
project('libapg', 'c',
  version: '1.1.0',
  default_options: ['c_std=c11']
)

libapg_sources = files(
  'src/package.c',
  'src/util.c',
  'src/archive.c',
  'src/crc32.c'
)

libapg_inc = include_directories('include')

libarchive_dep = dependency('libarchive', required: false)

libiron_dep = dependency('libiron', required: false)

if not libiron_dep.found()
  cc = meson.get_compiler('c')
  libiron_dep = cc.find_library('iron', required: true)
endif

if not libarchive_dep.found()
  cc = meson.get_compiler('c')
  libarchive_dep = cc.find_library('archive', required: true)
endif

libapg = library('apg', libapg_sources,
  include_directories: libapg_inc,
  dependencies: [
    libarchive_dep,
    libiron_dep
  ],
  install: true,
)

libapg_dep = declare_dependency(
  link_with: libapg,
  include_directories: libapg_inc,
)
