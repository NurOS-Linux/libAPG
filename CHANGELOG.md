# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added

- Meson wrap files (`subprojects/yyjson.wrap`, `subprojects/lmdb.wrap`, `subprojects/libarchive.wrap`) for automatic fallback subproject dependency building
- Support for extracting `.tar.gz` and `.tar.zstd` package archives alongside `.tar.xz` in `src/archive.c`

### Changed

- Moved cross-compilation target files from root directory into `cross/` directory (`cross/cross-*.txt`)

### Fixed

- Disabled `openssl`, `xml2`, `expat`, `cng`, and `iconv` in `libarchive` subproject default options to prevent host OpenSSL header lookup during cross-compilation

## [1.10.1] - 2026-07-20

### Fixed

- Insert path separator when concatenating directory paths in `concat_dirs()` (`src/util.c`)
- Disable broken x86_64 SHA-256 hardware assembly backend (`arch/x86_64/sha256.S`), falling back to portable C implementation due to missing/misordered message-schedule steps before `SHA256MSG2`

## [1.10.0] - 2026-07-20

### Added

- Parallel dependency resolution (`dep_graph_resolve_parallel()`) using lock-free read-only graph traversal and POSIX threads
- Parallelized dependency resolution and conflict checking (`dep_graph_find_breaks()`) during transaction preparation (`trans_prepare()`)
- Unit test for parallel graph resolution (`test_parallel_resolve()`)
- Lightweight Forgejo CI/CD workflows and issue/PR templates under `.forgejo/`
- 32-bit assembly backends for the checksum module (CRC-32, MD5, SHA-256) on
  ARM (AArch32/ARMv7), x86 (i386/IA-32), RISC-V (RV32), MIPS32 and PowerPC,
  extending the existing 64-bit backends to 32-bit targets; each replaces the
  portable C fallback with a hand-written scalar transform selected at build
  time via `host_machine.cpu_family()`. The message-scheduling code assembles
  words byte-wise, so the backends are endian-independent (validated on both
  little- and big-endian MIPS and on big-endian PowerPC), and symbol
  addressing is position-independent so they link cleanly into the shared
  library
- Cross-compilation files (`cross-arm.txt`, `cross-i386.txt`,
  `cross-riscv32.txt`, `cross-mips.txt`, `cross-powerpc.txt`) and CI coverage:
  an `arch-asm` job that assembles and PIC-links every 32-bit backend, plus a
  full `armhf` cross-compile job
- FreeBSD install-script sandbox: `run_script()` now isolates scripts on
  FreeBSD using Capsicum capability mode (`cap_enter()`); the script binary is
  opened before entering capability mode and executed with `fexecve()`, and
  sandbox failure is fail-closed, matching the Linux `unshare()` guarantee
- CI job that builds and tests the library in a native FreeBSD x86_64 virtual
  machine, now with the `gpgme` signing backend installed so the preferred
  backend gets the same test coverage it has on Linux
- PowerPC64 assembly backends for the checksum module (CRC-32, MD5, SHA-256)
  in `arch/ppc64/` (big-endian) and `arch/ppc64le/` (little-endian), targeting
  the ELFv2 ABI on both — including big-endian, where it is not glibc's
  traditional default (see `cross-ppc64.txt`). Content is byte-identical
  between the two directories: the message schedule is assembled byte-wise
  (`lbz` only), so the algorithm itself is endian-independent, matching the
  existing 32-bit backends. Correctness was verified by cross-assembling and
  running a freestanding syscall-only test harness under `qemu-ppc64-static`
  and `qemu-ppc64le-static` against the standard CRC-32 check value,
  MD5(""), and SHA-256("abc") test vectors
- PowerPC32 little-endian assembly backend in `arch/ppc32le/`, content
  byte-identical to `arch/ppc32/` for the same reason (32-bit PowerPC's ABI
  does not distinguish endianness for calling conventions). Verified by
  cross-assembling and PIC-linking; execution could not be verified via QEMU
  since no `qemu-ppcle` user-mode emulator exists upstream for this
  combination — 32-bit little-endian PowerPC is not shipped by any current
  mainstream distribution
- `cross-ppc32le.txt`, `cross-ppc64.txt`, `cross-ppc64le.txt` cross-compilation
  files; `arch-asm` CI job extended to assemble and PIC-link all three new
  backends alongside the existing ones

### Changed

- Renamed `arch/powerpc/` to `arch/ppc32/` and `cross-powerpc.txt` to
  `cross-ppc32.txt` to disambiguate from the new `ppc32le`/`ppc64`/`ppc64le`
  directories; `meson.build` and the `arch-asm` CI job updated accordingly.
  `meson.build`'s PowerPC dispatch now also branches on
  `host_machine.endian()`, since `cpu_family()` alone does not distinguish
  `ppc`/`ppc32le` or `ppc64`/`ppc64le`

### Fixed

- `-D_GNU_SOURCE` is now only added on Linux; it was previously applied
  unconditionally, which is a glibc-specific feature macro with no meaning on
  BSD libc
- Default `keyring_dir` and `keys_dir` now resolve under `/usr/local/etc/apg/`
  on FreeBSD instead of `/etc/apg/`, following FreeBSD's `hier(7)` convention
  for locally-installed configuration
- README documents FreeBSD `pkg install` dependencies and the platform's
  sandbox and configuration-path differences

## [1.9.0] - 2026-06-22

### Added

- Epoch prefix support in version strings: `N:version` (e.g. `1:2.0`); epoch
  defaults to zero when absent and is compared before the version part,
  allowing correct ordering when upstream resets version numbering
- Exclusive write lock on database open: `db_open()` acquires a
  `flock(LOCK_EX | LOCK_NB)` on `db.lock` inside the database directory,
  preventing concurrent write access across processes; returns NULL immediately
  if the lock is already held
- Package hold/pin: `db_set_hold()` marks an installed package as held;
  `trans_prepare()` blocks `TRANS_OP_REMOVE` and `TRANS_OP_UPGRADE` on held
  packages and returns `TRANS_ERR_HELD`; inspect blocked operations with
  `trans_get_held_pkgs()`

## [1.8.0] - 2026-06-19

### Added

- CI cross-compilation jobs for aarch64 and riscv64
- Release workflow now produces `.apg` packages for all three architectures
- `db_verify()` — checks that every file recorded for each installed package
  exists on disk under a given root; returns a `db_verify_issue` array with
  per-package lists of missing paths
- `db_verify_free()` — companion free function for `db_verify()` results

### Fixed

- `TRANS_OP_REMOVE` now removes package files from disk before calling
  `db_remove()`; previously only the database record was deleted
- `db_owner()` replaced O(n×m) full-scan with an O(1) `mdb_get` lookup
  against a new `file_owner` LMDB index (`file_path → pkg_name`); the index
  is maintained automatically by `db_add()` and `db_remove()`
- `run_script()` now sandboxes the child process with isolated network, mount,
  UTS, and IPC namespaces on Linux via `unshare()`; sandbox failure is
  fail-closed (script is not executed)

### Changed

- Linux namespace sandbox in `run_script()` guarded behind `#ifdef __linux__`;
  on other POSIX platforms scripts run without isolation
- Hardcoded paths and LMDB map size extracted into Meson build options:
  `keyring_dir`, `keys_dir`, `tmp_dir`, `db_mapsize`
- Cross files (`cross-aarch64.txt`, `cross-riscv64.txt`) migrated `c_args` and
  `c_link_args` to `[built-in options]`; added `pkgconfig` to `[binaries]`

## [1.7.0] - 2026-06-17

### Added

- `db_get_orphans()` — returns auto-installed packages no longer required by
  any other package
- Config file preservation during upgrade: existing files listed in the `conf`
  field are saved before the new version is installed; the new version's config
  is renamed to `<path>.apg-new` and the user's original is restored
- Removal blocked by dependents: `trans_prepare()` now calls
  `db_get_dependents()` and populates `trans_blocked_remove`; inspect with
  `trans_get_blocked_removes()`
- File conflict detection in `trans_prepare()`: checks `db_owner()` for every
  file in an incoming package and populates `trans_file_conflict`; inspect with
  `trans_get_file_conflicts()`

### Changed

- Hardcoded paths and the LMDB map size extracted into Meson build options:
  `keyring_dir`, `keys_dir`, `tmp_dir`, `db_mapsize`

## [1.6.0] - 2026-06-16

### Added

- `db_search()` — case-insensitive substring search across package name and
  description
- `db_stats()` — O(1) installed package count plus total file count via
  file-index scan
- `trans_add_upgrade()` — upgrade operation in transactions; installs a new
  package version over the existing one in a single atomic step

## [1.5.1] - 2026-06-16

### Added

- `db_get_dependents()` — reverse dependency lookup; checks both direct name
  and virtual names from the `provides` field

### Fixed

- **Security:** archive extraction now sets `ARCHIVE_EXTRACT_SECURE_NODOTDOT`
  and `ARCHIVE_EXTRACT_SECURE_SYMLINKS` to prevent path traversal (Zip Slip)
  via malicious package archives
- `rollback_committed()` now removes installed files from disk on transaction
  failure, not just the database records
- `journal_write` uses an atomic sequence counter to prevent a data race under
  concurrent `db_add`/`db_remove` calls
- `copy_file` now checks `fwrite` return value and propagates write errors
  instead of silently returning success on partial writes
- `db_get_dependents` checks `strdup` return value to avoid storing NULL in
  the result array under OOM

## [1.5.0] - 2026-06-15

### Added

- Audit log for all package operations (`include/apg/audit.h`)
- `audit_read_all()` — public API to read the operation history via
  `db_handle *` without exposing LMDB internals
- `JOURNAL_ROLLBACK` operation type — rollback removes are now recorded
  separately from user-requested removes
- `uid` field in `journal_entry` — records the UID of the user who
  initiated each operation
- `explicit_op` field in `journal_entry` — distinguishes user-requested
  operations from automatic dependency installs and rollbacks
- Version recording for remove operations — `db_remove` now looks up the
  installed version before deletion and includes it in the journal entry

### Changed

- `journal_write` signature extended with `uid_t uid` and `bool explicit_op`
- `trans_commit` now suppresses duplicate journal writes from `db_add`/
  `db_remove` and journals each step directly with the correct `explicit`
  flag and rollback context

## [1.4.0] - 2026-06-06

### Added

- Database opaque handle, parallel writes, lock-free reads, and hook callbacks
- Keyring with trust chain verification
- Transaction API
- Version constraints in package dependencies
- File ownership query in the database

## [1.3.1] - 2026-05-28

### Changed

- Expanded test suite to 21 cases covering dependency resolution, cycle
  detection, and conflict handling

## [1.3.0] - 2026-05-27

### Added

- Dependency resolver with topological sort and conflict detection
- Transaction journal with automatic rollback on post-install failure
- Test suite migrated to Meson

## [1.2.0] - 2026-05-14

### Added

- PGP signing backend via gpgme; libsodium Ed25519 fallback backend
- Hardware-accelerated SHA-256, CRC32, and MD5 via hand-written ASM for
  x86\_64 (Intel syntax, Clang IAS), aarch64, and riscv64
- Meson cross-compilation files for x86\_64, aarch64, and riscv64
- Pre/post-install script execution support
- Package installation with file copying
- Database read functions
- JSON serialization for packages via yyjson
- MD5, CRC32, and SHA-256 checksum verification
- pkg-config file generation

### Changed

- Split `db.c` into separate read, write, owner, and journal modules
- Switched x86\_64 ASM from NASM to Intel syntax with GAS/Clang IAS
- Removed logging system; simplified `parse_package`
- Replaced legacy copyright headers with SPDX license identifiers

### Removed

- Unused repo module
- Incomplete `sign.c` stub
- `flake.lock` (intentionally untracked)
