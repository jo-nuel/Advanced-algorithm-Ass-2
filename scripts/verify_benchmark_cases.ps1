$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build"
$includeDirectory = Join-Path $projectRoot "include"
$caseSource = Join-Path $projectRoot "src\benchmark\benchmark_cases.cpp"
$testSource = Join-Path $projectRoot "tests\benchmark_cases_tests.cpp"
$testProgram = Join-Path $buildDirectory "benchmark_cases_tests.exe"

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
    $caseSource `
    $testSource `
    -o $testProgram

if ($LASTEXITCODE -ne 0) {
    throw "Benchmark case compilation failed with exit code $LASTEXITCODE."
}

& $testProgram
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark case verification failed with exit code $LASTEXITCODE."
}

Write-Host "Benchmark case verification passed."

