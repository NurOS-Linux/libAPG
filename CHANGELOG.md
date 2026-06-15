# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added

- CI cross-compilation jobs for aarch64 and riscv64
- Release workflow now produces `.apg` packages for all three architectures

### Changed

- Cross files (`cross-aarch64.txt`, `cross-riscv64.txt`) migrated `c_args` and
  `c_link_args` to `[built-in options]`; added `pkgconfig` to `[binaries]`

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
