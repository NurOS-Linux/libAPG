# libAPG Roadmap

## v2.0 — Stable public API

- [x] ABI stabilization and documentation
- [x] C++ bindings (header-only, `bindings/cpp/`)
- [ ] Python bindings
- [x] Correctness tests: installation unit tests, fuzzing (package JSON, dep constraints)
- [x] pkg-config and CMake find module
- [x] Drop the gpgme (OpenPGP) signing backend entirely; libsodium (Ed25519) becomes the only one

## Maybe in the future

- [ ] Atomic installation: all-or-nothing semantics
