---
name: upgrade-duckdb-extension
description: Upgrade the system_stats extension to a new DuckDB release. Use when the user asks to upgrade DuckDB, bump the duckdb submodule, sync to a new DuckDB tag (e.g. v1.5.2), or update the duckdb / extension-ci-tools submodules together.
---

# Upgrade system_stats to a new DuckDB release

The DuckDB and extension-ci-tools submodules must move together. Then build,
run the Rust and SQL test suites, and write a changelog entry.

## Inputs

Before starting, confirm the target DuckDB version (e.g. `v1.5.2`). Everything else is derived from it.

## Workflow

Track these as a checklist; do not skip ahead:

- 1. Pin duckdb submodule to tags/$TARGET
- 2. Pin extension-ci-tools submodule to $TARGET (same tag)
- 3. Run Rust checks: `cargo test` and
  `cargo clippy --all-targets -- -D warnings`
- 4. Build: `CMAKE_BUILD_PARALLEL_LEVEL=10 make reldebug`
- 5. Run SQL tests: `make test_reldebug`

## Reference: historical upgrade commits

- `97359a4` — `Upgrade duckdb v1.5.2`.
