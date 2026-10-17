param(
    [ValidateSet("smoke", "full")]
    [string]$Mode = "smoke",
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build"
$includeDirectory = Join-Path $projectRoot "include"
$benchmarkProgram = Join-Path $buildDirectory "selection_benchmark.exe"

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $projectRoot "results\$($Mode)_benchmark.csv"
} elseif (-not [System.IO.Path]::IsPathRooted($OutputPath)) {
    $OutputPath = Join-Path $projectRoot $OutputPath
}

$outputDirectory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force $buildDirectory | Out-Null
New-Item -ItemType Directory -Force $outputDirectory | Out-Null

$compilerCommand = Get-Command "g++" -ErrorAction SilentlyContinue
if ($compilerCommand) {
    $compiler = $compilerCommand.Source
} else {
    $compiler = "C:\msys64\ucrt64\bin\g++.exe"
}

if (-not (Test-Path -LiteralPath $compiler)) {
    throw "g++ was not found in PATH or at the expected MSYS2 UCRT64 location."
}

$compilerDirectory = Split-Path -Parent $compiler
$env:PATH = "$compilerDirectory;$env:PATH"

$sources = @(
    (Join-Path $projectRoot "benchmarks\selection_benchmark.cpp"),
    (Join-Path $projectRoot "src\benchmark\benchmark_cases.cpp"),
    (Join-Path $projectRoot "src\benchmark\input_generator.cpp"),
    (Join-Path $projectRoot "src\benchmark\benchmark_runner.cpp"),
    (Join-Path $projectRoot "src\deterministic_selection.cpp"),
    (Join-Path $projectRoot "src\randomised_quickselect.cpp")
)

& $compiler `
    -std=c++23 `
    -O2 `
    -DNDEBUG `
    -Wall `
    -Wextra `
    -Wpedantic `
    -Werror `
    -I $includeDirectory `
    @sources `
    -o $benchmarkProgram

if ($LASTEXITCODE -ne 0) {
    throw "Benchmark compilation failed with exit code $LASTEXITCODE."
}

& $benchmarkProgram $Mode $OutputPath
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark run failed with exit code $LASTEXITCODE."
}

Write-Host "Benchmark results written to $OutputPath"

