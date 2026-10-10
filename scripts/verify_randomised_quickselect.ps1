$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build"
$includeDirectory = Join-Path $projectRoot "include"
$implementationSource = Join-Path $projectRoot "src\randomised_quickselect.cpp"
$testSource = Join-Path $projectRoot "tests\randomised_quickselect_tests.cpp"
$testProgram = Join-Path $buildDirectory "randomised_quickselect_tests.exe"

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

# Keep the compiler runtime available when the generated program starts.
$compilerDirectory = Split-Path -Parent $compiler
$env:PATH = "$compilerDirectory;$env:PATH"

& $compiler `
    -std=c++23 `
    -Wall `
    -Wextra `
    -Wpedantic `
    -Werror `
    -I $includeDirectory `
    $implementationSource `
    $testSource `
    -o $testProgram

if ($LASTEXITCODE -ne 0) {
    throw "Quickselect compilation failed with exit code $LASTEXITCODE."
}

& $testProgram
if ($LASTEXITCODE -ne 0) {
    throw "Quickselect verification failed with exit code $LASTEXITCODE."
}

Write-Host "Quickselect verification passed."
