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

# Recalculate every seven-run median directly from the raw file. This catches a
# summary script that writes the right number of rows but calculates them badly.
$summaryByCase = @{}
foreach ($summaryRow in $caseRows) {
    $key = "$($summaryRow.Family)|$($summaryRow.N)|$($summaryRow.K)|$($summaryRow.Algorithm)"
    if ($summaryByCase.ContainsKey($key)) {
        throw "Duplicate case summary row: $key"
    }
    $summaryByCase[$key] = $summaryRow
}

$rawRows = @(Import-Csv -LiteralPath $inputPath)
foreach ($rawGroup in @($rawRows | Group-Object family, n, k, algorithm)) {
    $groupRows = @($rawGroup.Group)
    $key = "$($groupRows[0].family)|$($groupRows[0].n)|$($groupRows[0].k)|$($groupRows[0].algorithm)"
    if (-not $summaryByCase.ContainsKey($key)) {
        throw "Missing case summary row: $key"
    }

    $orderedTimes = @($groupRows.time_ns | ForEach-Object { [long]$_ } | Sort-Object)
    if ($orderedTimes.Count -ne 7) {
        throw "Expected seven raw timings for: $key"
    }
    if ([long]$summaryByCase[$key].MedianNs -ne $orderedTimes[3]) {
        throw "Incorrect median in case summary: $key"
    }
}

foreach ($speedupRow in $speedupRows) {
    $deterministicKey = "$($speedupRow.Family)|$($speedupRow.N)|$($speedupRow.K)|median-of-medians"
    $randomisedKey = "$($speedupRow.Family)|$($speedupRow.N)|$($speedupRow.K)|randomised-quickselect"
    $expectedSpeedup = [math]::Round(
        [double]$summaryByCase[$deterministicKey].MedianNs /
            [double]$summaryByCase[$randomisedKey].MedianNs,
        3
    )
    if ([double]$speedupRow.QuickselectSpeedup -ne $expectedSpeedup) {
        throw "Incorrect speedup for $($speedupRow.Family), n=$($speedupRow.N), k=$($speedupRow.K)."
    }
}

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
    [xml]$svg = Get-Content -LiteralPath $path -Raw
    if ($svg.DocumentElement.LocalName -ne "svg") {
        throw "Generated plot is not a valid SVG document: $fileName"
    }
}

Write-Host "Benchmark analysis verification passed."
