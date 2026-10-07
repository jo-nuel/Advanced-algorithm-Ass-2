# Deterministic and Randomized Selection

This project compares two algorithms for finding the kth smallest value in an
unsorted array:

- Deterministic selection using Median of Medians
- Randomized Quickselect

The comparison will focus on running time, consistency between repeated runs,
and how each algorithm responds to different input arrangements.

## Primary authors

- Median of Medians: Jonathan Immanuel
- Randomized Quickselect: To be added

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
- The Randomized Quickselect author will work on a separate branch.

## Build and verify

From PowerShell, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify.ps1
```

The script uses `g++` from `PATH` when available. On the project development
computer, it can also use the MSYS2 UCRT64 compiler at
`C:\msys64\ucrt64\bin\g++.exe`.

The repository currently contains the shared project foundation. Algorithm
implementations and their detailed tests will be added on their author branches.

