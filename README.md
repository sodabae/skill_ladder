# SimEngine Build & Test Guide

## Build options

This project uses CMake with C++17.

### Default build (normal development)

The default path builds only the application executable (`sim_eng`) with the project warning flags.

```bash
cmake -S . -B build
cmake --build build
```

### Coverage + unit tests build

Use the `COVERAGE` option to enable:

- GoogleTest/GoogleMock unit-test build (`sim_eng_tests`)
- Coverage instrumentation flags (`-O0 -g --coverage`) on GCC/Clang
- Test discovery with CTest

```bash
cmake -S . -B build-coverage -DCOVERAGE=ON
cmake --build build-coverage
```

When `COVERAGE=ON`, CMake resolves GoogleTest in this order:

1. System-installed package (`find_package(GTest)`)
2. Fallback fetch/build from upstream using `FetchContent`

## Running tests (coverage build)

```bash
ctest --test-dir build-coverage --output-on-failure
```

## Generating HTML coverage output

When `COVERAGE=ON`, a custom target named `coverage_html` is available if coverage tooling exists (`lcov` + `genhtml`, or `gcovr`). For the `lcov` path, the target first captures an **initial baseline** (so files with 0% execution are included), then runs CTest and `sim_eng_tests`, captures runtime coverage with a `gcov` tool matched to your compiler version, merges baseline + runtime traces, and generates HTML.

```bash
cmake --build build-coverage --target coverage_html
```

Output is written under the build directory:

- `build-coverage/coverage_html/index.html`

If your environment is missing coverage tools, CMake will warn that `coverage_html` could not be created.
