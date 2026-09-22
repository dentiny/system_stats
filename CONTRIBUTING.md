# Contributing

## Reporting bugs

Search the existing [issues](https://github.com/dentiny/system_stats/issues)
before opening a bug report. Include the operating system, architecture, DuckDB
version, query, expected output, and actual output.

## Building

Install stable Rust and Python 3, then run:

```sh
git submodule update --init
CMAKE_BUILD_PARALLEL_LEVEL=14 make reldebug
```

The resulting DuckDB CLI is `build/reldebug/duckdb`.

## Testing

Run all checks before submitting a pull request:

```sh
cargo test
cargo fmt --all -- --check
cargo clippy --all-targets -- -D warnings
make test_reldebug
```

Changes to a table function should include a SQLLogicTest under `test/sql`.
Changes to pure Rust logic should include a Rust unit test.

## Style

Follow standard Rust conventions and keep the code formatted with `rustfmt`.
Avoid panics in extension callbacks; return an error so DuckDB can report it as
a query error.

## Pull requests

Use a feature branch and keep each pull request focused. Describe user-visible
behavior changes and any platform-specific limitations. The distribution
workflow builds the extension for the supported DuckDB targets.

## Updating DuckDB

The extension should track the latest supported DuckDB release. Follow
`docs/UPDATING.md` to update the Cargo dependency, build tooling, workflow, and
target version together.
