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

The shared verifier runs both independent correctness suites: Randomized
Quickselect first, followed by Median of Medians.

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

## Deterministic selection interface

Jonathan's implementation uses this function:

```cpp
int deterministicSelect(std::vector<int>& values, std::size_t k);
```

`k` is zero based, so `k == 0` asks for the smallest value. The function
rearranges the input vector while searching. It throws `std::out_of_range` when
the vector is empty or `k` is outside the vector.

## Median of Medians approach

The deterministic algorithm works with one active range at a time:

1. Divide the active values into groups of at most five.
2. Sort each small group and collect its median.
3. Recursively select the median of those medians as the pivot.
4. Partition the active range into values smaller than, equal to, and greater
   than the pivot.
5. Continue only in the partition containing rank `k`.

Groups of five give the pivot a useful worst-case guarantee. At least half of
the full groups have medians on each side of the chosen pivot, and each of those
groups contributes at least three values on that side. This removes a constant
part of the active range each time. The resulting worst-case running time is
`O(n)`, although the extra work used to choose the pivot can make it slower than
Randomized Quickselect on ordinary inputs.

Three-way partitioning is used so that repeated values equal to the pivot are
handled in one step. This is especially important when an input contains many
copies of the same number.

The implementation rearranges values in place and uses `O(log n)` recursive
stack space while selecting the pivot. The main partition-narrowing loop is
iterative.

## Deterministic selection tests

The tests cover small arrays, invalid ranks, negative values, sorted and
reverse-sorted inputs, groups with an incomplete final group, and arrays with
many duplicates. Fixed-seed generated arrays are checked against `std::sort`,
which provides a simple reference answer for every requested rank.

## Shared benchmark

The benchmark compares both algorithms on the same generated arrays. It uses
six input families, five requested ranks, one untimed warm-up, and seven timed
repetitions. Algorithm order alternates between repetitions. Input generation,
array copying, random generator setup, and result checking are outside the
timed section.

Run the short end-to-end benchmark with:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_benchmarks.ps1 -Mode smoke
```

Run the full size sequence with:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_benchmarks.ps1 -Mode full
```

By default, raw results are written to `results/smoke_benchmark.csv` or
`results/full_benchmark.csv`. Use `-OutputPath` to choose another CSV path.

## Results and analysis

The completed experiment contains 330 benchmark cases and 4,620 timed runs.
The `results` directory includes the raw measurements, case medians, summaries
by input size, speedup comparisons, and three SVG plots. Randomized Quickselect
had the lower median time in every tested case. At one million elements, its
median speedup across the five ranks ranged from 2.31 times on random arrays
with duplicates to 6.92 times on reverse-sorted arrays.

`scripts/summarise_benchmarks.ps1` contains the calculations used to produce
the submitted tables and plots. It also checks that each case has seven runs
for both algorithms and that both algorithms returned the same selected value.
The script uses only PowerShell and does not require a plotting package.

These measurements were collected on one computer, so they describe the
tested inputs and environment rather than proving that one algorithm is always
faster on every system.

