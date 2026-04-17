# NurOS Ruzen42 2025
project('libapg', 'c',
  version: '1.1.0'
)

libapg_sources = files(
  'src/package.c',
  'src/util.c',
  'src/archive.c',
  'src/checksum/crc32.c',
  'src/checksum/md5.c',
  'src/checksum/sha256.c',
  'src/checksum/checksum.c',
  'src/install/copy.c',
  'src/install/install.c',
  'src/db.c',
  'src/json.c',
)

add_project_arguments('-D_GNU_SOURCE', language: 'c')

libapg_inc = include_directories('include')

cc = meson.get_compiler('c')

libarchive_dep = dependency('libarchive', required: false)
lmdb_dep = dependency('lmdb', required: false)
libyyjson_dep = dependency('yyjson', required: false)
gpgme_dep = dependency('gpgme', required: false)
sodium_dep = dependency('libsodium', required: false)

if not libarchive_dep.found()
  libarchive_dep = cc.find_library('archive', required: true)
endif

if not lmdb_dep.found()
  lmdb_dep = cc.find_library('lmdb', required: true)
endif

if not libyyjson_dep.found()
  libyyjson_dep = cc.find_library('yyjson', required: true)
endif

sign_dep = []
if gpgme_dep.found()
  libapg_sources += files('src/sign/pgp/pgp.c')
  sign_dep = [gpgme_dep]
elif sodium_dep.found()
  libapg_sources += files('src/sign/sodium/sodium.c')
  sign_dep = [sodium_dep]
else
  error('Either gpgme or libsodium is required for package signing')
endif

libapg = library('apg', libapg_sources,
  include_directories: libapg_inc,
  dependencies: [
    libarchive_dep,
    lmdb_dep,
    libyyjson_dep,
  ] + sign_dep,
  install: true,
)

libapg_dep = declare_dependency(
  link_with: libapg,
  include_directories: libapg_inc,
)

pkg = import('pkgconfig')
pkg.generate(libapg)
