project('libapg', 'c',
  version: '1.11.3',
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
  'src/checksum/crc32.c',
  'src/checksum/md5.c',
  'src/checksum/sha256.c',
  'src/checksum/checksum.c',
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
  'src/db/search.c',
  'src/db/stats.c',
  'src/db/verify.c',
  'src/json.c',
  'src/graph/graph.c',
  'src/graph/resolve.c',
  'src/graph/conflict.c',
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
cpu = host_machine.cpu_family()

if get_option('fuzzing').enabled()
  if cc.get_id() != 'clang'
    error('fuzzing requires clang (libFuzzer)')
  endif
  add_project_arguments('-fsanitize=fuzzer-no-link,address,undefined',
                        language: 'c')
  add_project_link_arguments('-fsanitize=address,undefined', language: 'c')
endif

hw_args = []
if cpu == 'x86_64'
  # arch/x86_64/sha256.S is disabled: its hardware SHA-256 transform
  # produces incorrect digests (missing/misordered message-schedule steps
  # before SHA256MSG2). Falls back to the portable C implementation until
  # fixed. See CHANGELOG.
  libapg_sources += files(
    'arch/x86_64/crc32.S',
    'arch/x86_64/md5.S',
  )
  hw_args = [
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
    '-msha', '-msse4.1', '-masm=intel',
  ]
elif cpu == 'aarch64'
  libapg_sources += files(
    'arch/aarch64/sha256.S',
    'arch/aarch64/crc32.S',
    'arch/aarch64/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
    '-march=armv8-a+crypto+crc',
  ]
elif cpu == 'riscv64'
  libapg_sources += files(
    'arch/riscv64/sha256.S',
    'arch/riscv64/crc32.S',
    'arch/riscv64/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
    '-march=rv64gc_zknh',
  ]
elif cpu == 'mips64'
  libapg_sources += files(
    'arch/mips64/sha256.S',
    'arch/mips64/crc32.S',
    'arch/mips64/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
    '-march=mips64r2',
  ]
elif cpu == 'arm'
  libapg_sources += files(
    'arch/arm/sha256.S',
    'arch/arm/crc32.S',
    'arch/arm/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
    '-march=armv7-a',
  ]
elif cpu == 'x86'
  libapg_sources += files(
    'arch/x86/sha256.S',
    'arch/x86/crc32.S',
    'arch/x86/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
  ]
elif cpu == 'riscv32'
  libapg_sources += files(
    'arch/riscv32/sha256.S',
    'arch/riscv32/crc32.S',
    'arch/riscv32/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
  ]
elif cpu == 'mips'
  libapg_sources += files(
    'arch/mips/sha256.S',
    'arch/mips/crc32.S',
    'arch/mips/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
    '-march=mips32r2',
  ]
elif cpu == 'ppc' and host_machine.endian() == 'big'
  libapg_sources += files(
    'arch/ppc32/sha256.S',
    'arch/ppc32/crc32.S',
    'arch/ppc32/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
  ]
elif cpu == 'ppc' and host_machine.endian() == 'little'
  libapg_sources += files(
    'arch/ppc32le/sha256.S',
    'arch/ppc32le/crc32.S',
    'arch/ppc32le/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
  ]
elif cpu == 'ppc64' and host_machine.endian() == 'little'
  libapg_sources += files(
    'arch/ppc64le/sha256.S',
    'arch/ppc64le/crc32.S',
    'arch/ppc64le/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
  ]
elif cpu == 'ppc64' and host_machine.endian() == 'big'
  # Targets the ELFv2 ABI (-mabi=elfv2, set in cross/cross-ppc64.txt), not glibc's
  # traditional ELFv1 default for big-endian ppc64.
  libapg_sources += files(
    'arch/ppc64/sha256.S',
    'arch/ppc64/crc32.S',
    'arch/ppc64/md5.S',
  )
  hw_args = [
    '-DAPG_SHA256_HW_AVAILABLE=1',
    '-DAPG_CRC32_HW_AVAILABLE=1',
    '-DAPG_MD5_HW_AVAILABLE=1',
  ]
endif

if hw_args.length() > 0
  add_project_arguments(hw_args, language: 'c')
endif

threads_dep = dependency('threads')

libarchive_dep = dependency('libarchive', fallback: ['libarchive', 'libarchive_dep'])
lmdb_dep = dependency('lmdb', fallback: ['lmdb', 'lmdb_dep'])
libyyjson_dep = dependency('yyjson', fallback: ['yyjson', 'yyjson_dep'])

gpgme_dep = dependency('gpgme', required: get_option('gpgme'))
sodium_dep = dependency('libsodium', required: false)
if not sodium_dep.found()
  sodium_dep = cc.find_library('sodium', required: true)
endif

# Both signing backends are built in and exposed side by side (libsodium
# under the plain sign_*/keyring_* names, gpgme under the _gpgme suffix);
# callers of libapg pick whichever they need. gpgme is optional (see the
# 'gpgme' meson option) for platforms where it can't be built/found.
libapg_sources += files(
  'src/sign/sodium/sodium.c',
  'src/sign/sodium/keyring.c',
)
sign_dep = [sodium_dep]
if gpgme_dep.found()
  add_project_arguments('-DAPG_HAVE_GPGME=1', language: 'c')
  libapg_sources += files(
    'src/sign/pgp/pgp.c',
    'src/sign/pgp/keyring.c',
  )
  sign_dep += [gpgme_dep]
endif

seccomp_dep = dependency('libseccomp', required: false)
if seccomp_dep.found()
  add_project_arguments('-DAPG_HAVE_SECCOMP=1', language: 'c')
endif

libapg = library('apg', libapg_sources,
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
  'include/apg/checksum.h',
  'include/apg/config.h',
  'include/apg/copy.h',
  'include/apg/crc32.h',
  'include/apg/db.h',
  'include/apg/graph.h',
  'include/apg/install.h',
  'include/apg/journal.h',
  'include/apg/json.h',
  'include/apg/keyring.h',
  'include/apg/md5.h',
  'include/apg/package.h',
  'include/apg/scripts.h',
  'include/apg/sha256.h',
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
