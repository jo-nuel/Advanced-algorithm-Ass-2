$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build"
$testSource = Join-Path $projectRoot "tests\project_smoke.cpp"
$testProgram = Join-Path $buildDirectory "project_smoke.exe"

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
    $testSource `
    -o $testProgram

if ($LASTEXITCODE -ne 0) {
    throw "Compilation failed with exit code $LASTEXITCODE."
}

& $testProgram
if ($LASTEXITCODE -ne 0) {
    throw "Verification failed with exit code $LASTEXITCODE."
}

Write-Host "Verification passed."

