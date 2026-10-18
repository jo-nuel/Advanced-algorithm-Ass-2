$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build"
$outputDirectory = Join-Path $buildDirectory "benchmark_analysis_check"
$inputPath = Join-Path $projectRoot "results\full_benchmark.csv"
$analysisScript = Join-Path $PSScriptRoot "summarise_benchmarks.ps1"

$resolvedBuild = [System.IO.Path]::GetFullPath($buildDirectory)
$resolvedOutput = [System.IO.Path]::GetFullPath($outputDirectory)
if (-not $resolvedOutput.StartsWith($resolvedBuild + [System.IO.Path]::DirectorySeparatorChar)) {
    throw "Analysis check output must stay inside the build directory."
}
if (Test-Path -LiteralPath $resolvedOutput) {
    Remove-Item -LiteralPath $resolvedOutput -Recurse -Force
}

& $analysisScript -InputPath $inputPath -OutputDirectory $resolvedOutput

$caseRows = @(Import-Csv -LiteralPath (Join-Path $resolvedOutput "benchmark_case_summary.csv"))
$sizeRows = @(Import-Csv -LiteralPath (Join-Path $resolvedOutput "benchmark_size_summary.csv"))
$speedupRows = @(Import-Csv -LiteralPath (Join-Path $resolvedOutput "benchmark_speedup_summary.csv"))
if ($caseRows.Count -ne 660) { throw "Expected 660 case summary rows, found $($caseRows.Count)." }
if ($sizeRows.Count -ne 132) { throw "Expected 132 size summary rows, found $($sizeRows.Count)." }
if ($speedupRows.Count -ne 330) { throw "Expected 330 speedup rows, found $($speedupRows.Count)." }

foreach ($fileName in @(
    "median_time_by_size.svg",
    "speedup_at_largest_size.svg",
    "rank_effect_at_largest_size.svg"
)) {
    $path = Join-Path $resolvedOutput $fileName
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing generated plot: $fileName"
    }
    if ((Get-Item -LiteralPath $path).Length -eq 0) {
        throw "Generated plot is empty: $fileName"
    }
}

Write-Host "Benchmark analysis verification passed."
