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
- `colin/randomized-quickselect` contains Colin's implementation work.

## Build and verify

From PowerShell, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify.ps1
```

The script uses `g++` from `PATH` when available. On the project development
computer, it can also use the MSYS2 UCRT64 compiler at
`C:\msys64\ucrt64\bin\g++.exe`.

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

