# Testing this extension
The `sql` directory contains the extension's
[SQLLogicTests](https://duckdb.org/dev/sqllogictest/intro.html).

Build DuckDB with the extension and run the tests with:

```bash
CMAKE_BUILD_PARALLEL_LEVEL=14 make reldebug
make test_reldebug
```

The test runner uses the extension statically linked into the DuckDB test binary.
