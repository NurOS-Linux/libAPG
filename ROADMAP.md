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

- [ ] ASan/UBSan/TSan pass over the full test suite (`meson test`), not just
      ad hoc scratch runs like this session's.
- [ ] Extend fuzzing coverage: `candidate_set`/`sat_model` builders directly
      (not just through the fuzz harness's synthetic package generator),
      and the non-SAT paths (`db/`, `install/`, `transaction/`) that have
      no fuzz target yet.
- [ ] Re-check every O(n) claim from the last few milestones against
      realistic package counts (thousands, not the synthetic benchmarks
      used to verify each fix in isolation).
- [ ] Review `src/transaction/prepare.c` and `src/graph/resolve.c` (largest,
      most nested files by loop count) for correctness, not just the perf
      angle already covered.
- [ ] `sat_solve()`'s plain DPLL has no clause learning; profile whether
      real (non-synthetic) dependency sets ever get close to the decision
      budget before deciding if CDCL is worth the complexity.

## Maybe in the future

- [ ] Atomic installation: all-or-nothing semantics
