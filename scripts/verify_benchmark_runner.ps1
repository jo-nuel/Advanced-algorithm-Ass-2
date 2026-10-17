$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build"
$includeDirectory = Join-Path $projectRoot "include"
$sources = @(
    (Join-Path $projectRoot "src\benchmark\input_generator.cpp"),
    (Join-Path $projectRoot "src\benchmark\benchmark_runner.cpp"),
    (Join-Path $projectRoot "src\deterministic_selection.cpp"),
    (Join-Path $projectRoot "src\randomised_quickselect.cpp"),
    (Join-Path $projectRoot "tests\benchmark_runner_tests.cpp")
)
$testProgram = Join-Path $buildDirectory "benchmark_runner_tests.exe"

$compilerCommand = Get-Command "g++" -ErrorAction SilentlyContinue
if ($compilerCommand) {
    $compiler = $compilerCommand.Source
} else {
    $compiler = "C:\msys64\ucrt64\bin\g++.exe"
}

if (-not (Test-Path -LiteralPath $compiler)) {
    throw "g++ was not found in PATH or at the expected MSYS2 UCRT64 location."
}

New-Item -ItemType Directory -Force $buildDirectory | Out-Null

$compilerDirectory = Split-Path -Parent $compiler
$env:PATH = "$compilerDirectory;$env:PATH"

& $compiler `
    -std=c++23 `
    -Wall `
    -Wextra `
    -Wpedantic `
    -Werror `
    -I $includeDirectory `
    @sources `
    -o $testProgram

if ($LASTEXITCODE -ne 0) {
    throw "Benchmark runner compilation failed with exit code $LASTEXITCODE."
}

& $testProgram
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark runner verification failed with exit code $LASTEXITCODE."
}

Write-Host "Benchmark runner verification passed."

