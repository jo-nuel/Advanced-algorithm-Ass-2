# Deterministic and Randomized Selection

This project compares two algorithms for finding the kth smallest value in an
unsorted array:

- Deterministic selection using Median of Medians
- Randomized Quickselect

The comparison will focus on running time, consistency between repeated runs,
and how each algorithm responds to different input arrangements.

## Primary authors

- Median of Medians: Jonathan Immanuel
- Randomized Quickselect: Colin Ice King-eo

Both members will contribute to testing, benchmarking, analysing the results,
writing the report, and producing the video.

## Planned project structure

```text
include/    Function declarations
src/        Algorithm implementations
tests/      Correctness tests
scripts/    Build and verification scripts
```

## Development branches

- `main` contains reviewed shared work.
- `jonathan/median-of-medians` contains Jonathan's implementation work.
- `colin/randomised-quickselect` contains Colin's implementation work.

## Build and verify

From PowerShell, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify.ps1
```

The script uses `g++` from `PATH` when available. On the project development
computer, it can also use the MSYS2 UCRT64 compiler at
`C:\msys64\ucrt64\bin\g++.exe`.

The shared verifier runs Quickselect's tests on this branch. When the Median of
Medians implementation and tests are also present, it runs both independent
correctness suites.

## Randomised Quickselect

- API and contract: `include/selection/randomised_quickselect.hpp`
- Implementation: `src/randomised_quickselect.cpp`
- Standalone correctness tests: `tests/randomised_quickselect_tests.cpp`

`selection::randomisedSelect(values, k)` returns the kth smallest integer using
zero-based `k`: zero selects the minimum and `values.size() - 1` selects the
maximum. Duplicate values count as separate elements. Selection may rearrange
the vector, preserving its size and multiset; it does not guarantee sorted
output. Empty input or `k >= values.size()` throws `std::out_of_range` before
modifying the vector.

The overload accepting `std::mt19937&` supports reproducible testing without
global RNG state. Identical pivot sequences across different standard-library
implementations are not guaranteed. Running time is expected O(n), worst-case
O(n²), with O(1) auxiliary algorithm storage.

To build and run only the Quickselect correctness suite:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_randomised_quickselect.ps1
```

The scripts compile with `-std=c++23 -Wall -Wextra -Wpedantic -Werror` and use
`include` as the header search directory. Quickselect's executable is
`build/randomised_quickselect_tests.exe`. These commands check correctness;
benchmarking and the comprehensive algorithm comparison remain deferred.

