# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added

- `test_parse_package_install_roundtrip` and `test_install_package_in_root_uses_isolated_temp_dirs` (`test/src/test_install.c`): end-to-end `parse_package()`/`install_package_in_root()`/`package_collect_files()` tests against real built `.apg` archives, the latter a regression test for the per-call temp directory isolation fixed in 1.11.3
- Proper pkg-config metadata: `pkg.generate()` now sets `name`/`filebase` to `libapg` (previously defaulted to the library target name `apg`, so consumers had to look up `apg.pc` instead of the documented project name), plus `description` and `url` (`meson.build`)
- CMake support (`cmake/libapg-config.cmake`, installed to `<libdir>/cmake/libapg/`): `find_package(libapg REQUIRED)` resolves via pkg-config and exposes the `libapg::libapg` imported target

### Changed

- CI/release workflows (`.github/workflows/`, `.forgejo/workflows/`) no longer build `yyjson` from an unpinned `git clone` of upstream HEAD (or, on FreeBSD, the `pkg` system package); every job now falls through to the pinned, hash-verified `subprojects/yyjson.wrap` fallback that `meson.build` already declares, so `yyjson` comes from exactly one, integrity-checked source on every platform
- `ci.yml` now declares a top-level `permissions: contents: read` (both mirrors); previously the workflow ran with whatever the repository's default `GITHUB_TOKEN` permissions were, wider than any of its read-only build/test/sanitizer/cross-compile jobs need
- `.forgejo/workflows/release.yml` now declares `permissions: contents: write`, matching the equivalent GitHub workflow (previously unset there, relying on the Forgejo instance's default token scope for the release-creation job)
- All third-party Actions in `.github/workflows/` and `.forgejo/workflows/` (`actions/checkout`, `actions/upload-artifact`, `actions/download-artifact`, `actions/upload-pages-artifact`, `actions/deploy-pages`, `cross-platform-actions/action`, `softprops/action-gh-release`) are now pinned to a resolved commit SHA instead of a mutable version tag (e.g. `@v4`), with the corresponding version kept as a trailing comment; a compromised or re-pointed tag on any of these can no longer silently change what code runs in CI
- **Breaking:** `struct trans_step`, `struct trans_conflict`, `struct trans_file_conflict`, `struct trans_held_pkg`, `struct trans_blocked_remove` (`include/apg/transaction.h`), `struct db_verify_issue` (`include/apg/db.h`), and `struct journal_entry` (`include/apg/journal.h`) are now opaque; read their fields with the new `trans_step_*()`, `trans_conflict_*()`, `trans_file_conflict_*()`, `trans_held_pkg_*()`, `trans_blocked_remove_*()`, `db_verify_issue_*()`, and `journal_entry_*()` accessors instead of direct field access. `trans_get_plan()`, `trans_get_conflicts()`, `trans_get_blocked_removes()`, `trans_get_file_conflicts()`, and `trans_get_held_pkgs()` are replaced by `trans_plan_count()`/`trans_plan_at()` and the equivalent `_count()`/`_at()` pairs, since an opaque type can no longer be indexed as a raw array. `db_verify()` and `journal_read_all()` keep their existing signatures. This is the first step of the pre-2.0 ABI-stabilization pass tracked in `ROADMAP.md`: these were all read-only "result" types the caller never constructs, so hiding their layout up front avoids an ABI break later if they ever gain fields. `struct package`, `struct package_metadata`, and other types tied to the on-disk `.apg` package format are intentionally not part of this pass and are not expected to change

### Fixed

- `libapg_dep` (`meson.build`) now carries `lmdb_dep`'s include path; `include/apg/journal.h` includes `<lmdb.h>` directly (for `MDB_env`), but nothing depending on the in-tree `libapg_dep` object got lmdb's headers on its include path unless it happened to come from elsewhere. Went unnoticed until `test/src/test_accessors.c` became the first test to pull in `<apg/audit.h>` → `<apg/journal.h>`, breaking the FreeBSD CI job. External consumers via pkg-config were never affected (`Requires.private: lmdb` already covers them)
- `copy_file()`/`copy_dir()` (`src/install/copy.c`), used by `install_data_dir()`/`install_home_dir()` to copy a package's already-extracted files into the real install root, never applied the source file's/directory's permission bits to the destination — every installed file and directory silently got whatever default mode `fopen()`/`mkdir()` produced (e.g. executables losing their `+x` bit), even though the earlier libarchive extraction step into the temp directory had preserved them correctly. Both functions now `chmod()` the destination to match the source's mode bits after creating it; for directories, only ones newly created by this call are re-chmod'd, so pre-existing shared directories (`/usr/bin`, `/etc`, ...) are never touched
- `dep_graph_find()`/`dep_graph_lookup()` (`src/graph/graph_priv.h`), internal cross-file helpers with no declaration in any public header, were nonetheless exported from `libapg.so`'s dynamic symbol table by default (no visibility control was in place). Marked `__attribute__((visibility("hidden")))`; verified via `nm -D` that every remaining exported symbol now corresponds to a declaration in `include/apg/*.h`/`include/util.h`

### Removed

- **Breaking:** all checksum verification. `include/apg/checksum.h`, `include/apg/crc32.h`, `include/apg/md5.h`, `include/apg/sha256.h`, `src/checksum/`, and every architecture's `arch/*/{crc32,md5,sha256}.S` backend are gone; `install_package_in_root()` (`src/package.c`) no longer calls `verify_checksums()`, and the `crc32sums`/`md5sums`/`sha256sums` files inside a `.apg` archive are no longer read or required. Package integrity/authenticity is provided solely by signature verification. Removed the corresponding tests (`test_checksum.c`, `test_checksum_fuzz.c`) and docs pages (`docs/api/checksum.rst`, `docs/api/crypto.rst`), and dropped the now-dead `arch-asm` CI job and the release workflows' `md5sums` generation
- **Breaking:** the gpgme (OpenPGP) signing backend, completing the removal tracked in `ROADMAP.md`. `sign_verify_gpgme()`, `sign_file_gpgme()` (`include/apg/sign.h`), `keyring_load_gpgme()`, `keyring_verify_gpgme()`, `keyring_free_gpgme()`, `keyring_add_key_gpgme()`, `struct keyring_gpgme` (`include/apg/keyring.h`), `sign_backend_t`/`SIGN_BACKEND_SODIUM`/`SIGN_BACKEND_GPGME`, and `install_policy.backend` (`include/apg/config.h`) are gone; `src/sign/pgp/` is deleted. libsodium (Ed25519) is now the only signing backend — `trans_commit()` always verifies against it, there is no backend selection left to make. The `gpgme` meson option and gpgme package dependency are removed from `meson.build`/`meson_options.txt` and every CI/release workflow

## [1.11.3] - 2026-07-22

### Fixed

- `parse_package()`, `package_collect_files()`, and `install_package_in_root()` (`src/package.c`) now extract each package into a unique, per-call temp directory instead of the same fixed shared path; previously a package's recorded file list could silently include leftover files from any prior package extracted into that same directory, causing spurious `TRANS_ERR_FILE_CONFLICT` errors and incorrect ownership records
- Added `remove_dir_recursive()` (`include/util.h`, `src/util.c`) and use it to clean up each package's temp extraction directory after use

## [1.11.2] - 2026-07-22

### Fixed

- CI: install missing `libsodium-dev`/`libgpgme-dev` across all ISA jobs (native x86_64, aarch64, riscv64, armhf); build `libgpg-error`/`libassuan`/`gpgme` from source for mips64el
- Read `metadata.json` instead of `meta.json` when parsing a package archive (`src/package.c`), matching the documented APG format spec
- Expand `$HOME` in `conf` entries against the invoking user's real home directory instead of treating it as a literal path component (`src/transaction/commit.c`)
- Persist a package's `scripts/` directory at install time so `pre-remove`/`post-remove` actually run on removal (`include/apg/scripts.h`, `src/install/scripts.c`, `src/transaction/commit.c`); previously these were documented but never invoked

## [1.11.1] - 2026-07-21

### Fixed

- Fixed `run_script()` failing to execute any install script on FreeBSD, which caused the `test_run_script_root` `SIGABRT` in CI (`src/install/scripts.c`). Two issues stacked up:
  - `cap_enter()` was entering Capsicum capability mode before `fexecve()`, but capability mode forbids the kernel from looking up an interpreter path, which any `#!`-script exec requires. Removing `cap_enter()` alone did not fix it.
  - Independently of Capsicum, FreeBSD's `fexecve()` cannot execute interpreted (`#!`) scripts at all: the shell image activator needs a real pathname to build the interpreter's argv, not just a file descriptor.
  - The FreeBSD-specific `open()` + `fexecve()` branch is removed; FreeBSD now falls through to the same path-based `execve()`/`chroot()` logic already used for non-Linux targets, matching what Linux does with `execl()`. Containment for an alternate install root is still provided by `chroot()`

## [1.11.0] - 2026-07-21

### Added

- Meson wrap files (`subprojects/yyjson.wrap`, `subprojects/lmdb.wrap`, `subprojects/libarchive.wrap`) for automatic fallback subproject dependency building
- Support for extracting `.tar.gz` and `.tar.zstd` package archives alongside `.tar.xz` in `src/archive.c`
- gpgme and libsodium signing backends are now built into `libapg` side by side instead of picking one at build time: libsodium keeps the plain `sign_verify`/`sign_file`/`keyring_load`/`keyring_verify`/`keyring_free`/`keyring_add_key`/`struct keyring` names (`include/apg/sign.h`, `include/apg/keyring.h`), and gpgme is exposed under a `_gpgme` suffix (`sign_verify_gpgme`, `sign_file_gpgme`, `keyring_load_gpgme`, `keyring_verify_gpgme`, `keyring_free_gpgme`, `keyring_add_key_gpgme`, `struct keyring_gpgme`)
- `sign_backend_t` enum (`SIGN_BACKEND_SODIUM`, `SIGN_BACKEND_GPGME`) and a new `backend` field on `install_policy` (`include/apg/config.h`), defaulting to `SIGN_BACKEND_SODIUM`, so callers can pick which keyring backend `trans_commit()` verifies package signatures against (`src/transaction/commit.c`)

### Changed

- Moved cross-compilation target files from root directory into `cross/` directory (`cross/cross-*.txt`)
- `gpgme` and `libsodium` are now both required build dependencies instead of an either/or choice; `meson.build` always links both signing backends into `libapg.so`

### Fixed

- Disabled `openssl`, `xml2`, `expat`, `cng`, and `iconv` in `libarchive` subproject default options to prevent host OpenSSL header lookup during cross-compilation
- Fixed a potential stack buffer overflow in `verify_crc32sums()`, `verify_md5sums()`, and `verify_sha256sums()` (`src/checksum/checksum.c`): the `sscanf()` field width for the path column was hardcoded to `4095`, assuming a 4096-byte `PATH_MAX`, but the destination buffer is sized to the platform's actual `PATH_MAX` (1024 on FreeBSD), so a long path in a `sums` file could overflow it. The width is now computed from the buffer's actual size

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
