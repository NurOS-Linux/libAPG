# libAPG Roadmap

## v2.0 — Stable public API

- [x] ABI stabilization and documentation
- [x] C++ bindings (header-only, `bindings/cpp/`)
- [x] Python bindings
- [x] Correctness tests: installation unit tests, fuzzing (package JSON, dep constraints)
- [x] pkg-config and CMake find module
- [x] Drop the gpgme (OpenPGP) signing backend entirely; libsodium (Ed25519) becomes the only one

## v2.2.0 — SAT-based dependency solver

Additive only: existing `dep_graph_resolve()` / `dep_graph_resolve_parallel()`
keep their current behavior and ABI. This ships as a new, separate resolve
entry point; it does not touch `.apg`, `struct package`, or
`struct package_metadata`.

- [x] `dep_graph` today maps one name to exactly one node (`node_map`,
      `alias_map`); it cannot represent multiple candidate versions/providers
      for the same name. Needs a parallel candidate-set representation
      usable by the solver without disturbing the existing single-candidate
      graph used by the current resolvers.
- [x] Constraint model: turn each package's `dependencies` (name + version
      op) and `conflicts` into SAT clauses over candidate selection
      variables.
- [x] Provider/alias handling: a `provides`/`replaces` alias can be satisfied
      by any of several candidates — needs "at least one of" clauses instead
      of today's single deterministic pick. Falls out of the unified
      `candidate_set` model (a `provides` name and a real package name share
      the same lookup) — no separate handling needed, verified with a
      multi-provider dependency clause. `replaces` is not covered yet.
- [x] Solver core: pick and implement an actual SAT (or CDCL-lite/PubGrub-style
      incremental) algorithm; decide on backtracking strategy and a search
      budget/timeout for pathological inputs. Shipped as a plain DPLL solver
      (unit propagation + backtracking, first-unassigned-variable decision
      order, decision-count budget) in `src/graph/sat_solve.c`. No clause
      learning (not CDCL) — a reasonable first cut, not the final version.
      Note: `sat_model_build()` alone doesn't force any package to be
      installed — the caller must add a unit clause for each root package it
      actually wants, same as the `[app forced]` case used to verify this.
- [x] Conflict reporting: `sat_solve()` returns a human-readable explanation
      string on UNSAT (`src/graph/sat_solve.c`), not just an error code.
- [x] New public API surface: `dep_graph_resolve_sat()` (`include/apg/graph.h`,
      `src/graph/sat.c`), additive, no changes to existing signatures.
      Wraps `candidate_set`/`sat_model`/`sat_solve_parallel()` (all still
      private) behind a package-pointer-in, package-pointer-out interface,
      callers never see var ids or clauses. Tested from `test/apg-test`
      using only public headers.
- [x] Threading: `sat_solve_parallel()` (`src/graph/sat_solve.c`) runs a
      portfolio of DPLL searches over the same `sat_model` (varied
      decision order/polarity per thread), first definitive result wins,
      others cancelled via an atomic flag checked in `dpll()`. Sharding by
      root name (like `dep_graph_resolve_parallel()`) was rejected: it
      would hide cross-root conflicts, the whole reason for using SAT.
      Race-checked with ThreadSanitizer, cross-checked against
      `sat_solve()` via fuzzing.

- [x] Tests: unit tests (`test/src/test_sat.c`, `sat-test`)
- [x] Fuzzing (`fuzz/fuzz_sat_model.c`, `fuzz-sat-model`)
- [x] `sat_model_var()` was a linear scan, making `sat_model_build()` O(n²);
      indexed by pointer hash, now O(n). 4000 pkgs: 9.04ms → 0.98ms.
- [x] Perf benchmark vs. current resolver: won't do. The two solve
      different problems (single deterministic candidate per name vs.
      joint multi-candidate/multi-root search), so a general comparison
      wouldn't be meaningful.

## v2.3.0 — Bug hunting and performance

No new features. Audit what's already shipped.

- [x] ASan/UBSan/TSan pass over the full test suite (`meson test`), not just
      ad hoc scratch runs like this session's. Found two real infra bugs
      along the way: `scripts/checkpatch.py`'s clang-format check only
      excluded `build/`, not `build-*/`, so it picked up stray files from
      scratch sanitizer build dirs (fixed). `apgpy-bindings` failed under
      ASan/TSan because ctypes dlopen()s the sanitizer-instrumented `.so`
      into a plain `python3` process with no runtime preloaded
      (`bindings/python/meson.build` now sets `LD_PRELOAD` when
      `b_sanitize` is active). One TSan failure in `test_run_script_root`
      traced to a real cause, not a libapg bug: `unshare(CLONE_NEWUSER)`
      requires a single-threaded caller, and TSan's runtime spawns
      background threads even in code that looks single-threaded, so
      `EINVAL`. That code path can't be tested under TSan, full stop.
- [x] `candidate_set` fuzzed directly: `fuzz/fuzz_candidate_set.c`
      (`fuzz-candidate-set`) feeds raw fuzzer bytes straight into
      `candidate_set_add()`/`candidate_set_lookup()`, no synthetic-package
      layer in between. 564k execs in a 25s local run, zero crashes.
- [x] `sat_model` builder fuzzed directly: `fuzz/fuzz_sat_model_vars.c`
      (`fuzz-sat-model-vars`) drives `sat_model_var()`/`sat_model_force()`
      straight from fuzzer bytes against a fixed pool of dummy packages, no
      package generator. 365k execs in 25s, zero crashes.
- [x] Non-SAT paths now have fuzz targets: `fuzz/fuzz_db.c`
      (`fuzz-db`, `db_add()`/`db_get()` round-trip, 310k execs/30s, zero
      crashes), `fuzz/fuzz_install_data_dir.c` (`fuzz-install-data-dir`,
      synthetic file trees into an isolated temp root, 57.8k execs/20s,
      zero crashes), `fuzz/fuzz_transaction_prepare.c`
      (`fuzz-transaction-prepare`, synthetic multi-package
      dependency/conflict scenarios through `trans_prepare()`, 40.3k
      execs/25s, zero crashes).
- [x] Re-checked at n=1000..20000: `db_get_orphans()` and `sat_model_build()`
      both hold linear (20k pkgs: 60.82ms / 3.82ms). First attempt at
      re-checking `dep_graph_resolve_parallel()`'s merge fix used a bad
      benchmark (resolving every node of an n-length chain as its own
      root, which is inherently O(n²) DFS work regardless of merge cost,
      not a regression). Redone isolating just the merge (many roots
      sharing one dependency): clean linear scaling, 20k roots in
      480.56ms.
- [x] Reviewed `src/transaction/prepare.c` and `src/graph/resolve.c` for
      correctness. Found and verified with a standalone repro: upgrade
      targets' dependencies are never validated. `trans_prepare()` calls
      `dep_graph_resolve_parallel()` only over `trans->install_pkgs`
      names, never `trans->upgrade_pkgs`, so an upgrade that adds a new
      required dependency not already installed silently returns
      `TRANS_OK` instead of `TRANS_ERR_MISSING_DEP`. Not fixed yet, needs
      its own design pass (whether/how to fold upgrade targets into the
      resolve step). `resolve.c`'s concurrent-read safety during
      `dep_graph_resolve_parallel()` also checked: `str_map_get()` never
      mutates, so parallel lookups from multiple threads are safe.
- [x] Profiled `sat_solve()`'s decision budget against a non-pathological,
      DAG-shaped dependency graph (branching deps, shared virtual
      providers, no adversarial structure) up to 3000 packages: solves in
      under 3ms, nowhere near the decision budget. CDCL is not justified
      by anything seen on realistic-shaped input; the earlier PHP(n)
      slowdown was a deliberately adversarial worst case, not
      representative.

## Maybe in the future

- [ ] Atomic installation: all-or-nothing semantics
