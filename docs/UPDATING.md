# Updating DuckDB

This extension uses DuckDB's C++ extension entrypoint with a Rust static
library for its implementation.

To update:

1. Update the `duckdb` submodule to the desired release.
2. Update `duckdb_version`, `ci_tools_version`, and the reusable workflow tag
   in `.github/workflows/MainDistributionPipeline.yml`.
3. Update the `extension-ci-tools` submodule to the matching release branch.
4. Run `cargo test`, `cargo clippy --all-targets -- -D warnings`,
   `make reldebug`, and `make test_reldebug`.

Review DuckDB's C++ extension API changes when upgrading because that API is
version-specific.