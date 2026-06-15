# libAPG Roadmap

## v1.1 (current)

- [x] Package metadata parsing (JSON)
- [x] File, script, and config installation
- [x] Installed package database (LMDB)
- [x] Checksums: CRC32, MD5, SHA-256
- [x] Package signing: PGP (gpgme) / libsodium
- [x] Hardware-accelerated SHA-256 (x86_64 SHA-NI, AArch64 crypto, RISC-V Zknh)
- [x] Optimized assembly for CRC32 and MD5 on all three architectures

## v1.2 — Dependencies and conflicts

- [x] Dependency resolver (graph, topological sort)
- [x] Conflict and circular dependency detection at install time
- [x] `provides` / `replaces` support during resolution
- [x] API to query: what installing package X will break

## v1.3 — Transactions and rollback

- [x] Rollback on post-install script failure
- [x] Transaction journal in the database

## v1.4 — Database API

- [x] Public API for third-party tools to register installed packages
- [x] Conflict-free parallel installs: no "package not installed" or "file busy" errors when multiple tools write to the DB simultaneously
- [x] Lock-free read path for queries (list, search, get)
- [x] Hooks: pre/post DB write callbacks for tools like custom package helpers

## v1.5 — Security

- [x] Trust chain verification (keyring)
- [x] Install policies: reject unsigned packages, trusted key list
- [x] Sandboxed execution of install scripts (seccomp / namespaces)
- [x] Audit log of all package operations

## v2.0 — Stable public API

- [ ] ABI stabilization and documentation
- [ ] Bindings for other languages (Python, C++)
- [ ] Correctness tests: checksum fuzzing, installation unit tests
- [ ] pkg-config and CMake find module

## Maybe in the future

- [ ] Atomic installation: all-or-nothing semantics
