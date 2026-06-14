# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

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
