# libAPG Roadmap

## v2.0 — Stable public API

- [ ] ABI stabilization and documentation
- [ ] Bindings for other languages (Python, C++)
- [x] Correctness tests: checksum fuzzing, installation unit tests
- [x] pkg-config and CMake find module
- [ ] Drop the gpgme (OpenPGP) signing backend entirely; libsodium (Ed25519) becomes the only one

## Maybe in the future

- [ ] Atomic installation: all-or-nothing semantics
