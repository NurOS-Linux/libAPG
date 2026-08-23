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
- [ ] Conflict reporting: on UNSAT, produce a human-readable explanation
      (which constraints clashed), not just an error code.
- [ ] New public API surface (new header declarations, new exported
      symbols) — additive, `APG_API`, no changes to existing signatures.
- [ ] Threading: decide whether/how this interacts with the existing
      `pthread`-parallel resolve path, or whether the solver stays
      single-threaded initially.
- [ ] Tests: unit tests for satisfiable/unsatisfiable cases (done —
      `test/src/test_sat.c`, registered as `sat-test` in `meson test`:
      dependency resolution, version-constrained candidates, multi-provider
      "at least one", conflict-forced UNSAT, and decision-budget exhaustion),
      fuzzing over randomly generated constraint sets (not started), and a
      perf benchmark against the current resolver on typical
      (non-pathological) dependency sets (not started — also not directly
      comparable yet, the SAT path and `dep_graph_resolve()` solve different
      problems).

## Maybe in the future

- [ ] Atomic installation: all-or-nothing semantics
