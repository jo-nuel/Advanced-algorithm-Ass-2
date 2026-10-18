param(
    [string]$InputPath,
    [string]$OutputDirectory
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $InputPath) {
    $InputPath = Join-Path $projectRoot "results\full_benchmark.csv"
}
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $projectRoot "results"
}

function Get-Median {
    param([double[]]$Values)

    if ($Values.Count -eq 0) {
        throw "Cannot calculate the median of an empty set."
    }

    $ordered = @($Values | Sort-Object)
    $middle = [int][math]::Floor($ordered.Count / 2)
    if ($ordered.Count % 2 -eq 1) {
        return [double]$ordered[$middle]
    }
    return ([double]$ordered[$middle - 1] + [double]$ordered[$middle]) / 2.0
}

function Get-RankLabel {
    param([int]$Size, [int]$Rank)

    if ($Rank -eq 0) { return "minimum" }
    if ($Rank -eq [int]($Size / 4)) { return "quarter" }
    if ($Rank -eq [int]($Size / 2)) { return "median" }
    if ($Rank -eq [int](3 * $Size / 4)) { return "three-quarter" }
    if ($Rank -eq $Size - 1) { return "maximum" }
    throw "Rank $Rank is not one of the five benchmark ranks for size $Size."
}

function Add-SvgText {
    param(
        [System.Text.StringBuilder]$Builder,
        [double]$X,
        [double]$Y,
        [string]$Text,
        [int]$Size = 12,
        [string]$Anchor = "start",
        [string]$Weight = "normal"
    )

    $line = (
        '<text x="{0:F1}" y="{1:F1}" font-family="Arial, sans-serif" font-size="{2}" ' +
        'font-weight="{3}" text-anchor="{4}" fill="#111111">{5}</text>'
    ) -f $X, $Y, $Size, $Weight, $Anchor, $Text
    [void]$Builder.AppendLine($line)
}

function Add-SvgLine {
    param(
        [System.Text.StringBuilder]$Builder,
        [double]$X1,
        [double]$Y1,
        [double]$X2,
        [double]$Y2,
        [string]$Colour,
        [double]$Width = 1.0
    )

    $line = (
        '<line x1="{0:F1}" y1="{1:F1}" x2="{2:F1}" y2="{3:F1}" ' +
        'stroke="{4}" stroke-width="{5:F1}" />'
    ) -f $X1, $Y1, $X2, $Y2, $Colour, $Width
    [void]$Builder.AppendLine($line)
}

function Start-Svg {
    param([int]$Width, [int]$Height, [string]$Title)

    $builder = [System.Text.StringBuilder]::new()
    [void]$builder.AppendLine('<?xml version="1.0" encoding="UTF-8"?>')
    $openingTag = (
        '<svg xmlns="http://www.w3.org/2000/svg" width="{0}" height="{1}" viewBox="0 0 {0} {1}">' -f
        $Width, $Height
    )
    [void]$builder.AppendLine($openingTag)
    [void]$builder.AppendLine('<rect width="100%" height="100%" fill="#ffffff" />')
    Add-SvgText $builder ($Width / 2) 32 $Title 20 "middle" "bold"
    return $builder
}

function Save-Svg {
    param([System.Text.StringBuilder]$Builder, [string]$Path)

    [void]$Builder.AppendLine('</svg>')
    [System.IO.File]::WriteAllText($Path, $Builder.ToString(), [System.Text.UTF8Encoding]::new($false))
}

function Get-SizeLabel {
    param([int]$Size)
    if ($Size -ge 1000000) { return "{0:g}m" -f ($Size / 1000000) }
    if ($Size -ge 1000) { return "{0:g}k" -f ($Size / 1000) }
    return [string]$Size
}

function Get-TimeLabel {
    param([double]$Nanoseconds)
    $milliseconds = $Nanoseconds / 1000000.0
    if ($milliseconds -ge 10) { return "{0:F0} ms" -f $milliseconds }
    if ($milliseconds -ge 1) { return "{0:F1} ms" -f $milliseconds }
    return "{0:F2} ms" -f $milliseconds
}

if (-not (Test-Path -LiteralPath $InputPath -PathType Leaf)) {
    throw "Benchmark input was not found: $InputPath"
}

$rawRows = @(Import-Csv -LiteralPath $InputPath)
if ($rawRows.Count -eq 0) {
    throw "Benchmark input contains no rows."
}

$requiredColumns = @(
    "family", "n", "k", "input_seed", "pivot_seed",
    "algorithm", "repetition", "time_ns", "result"
)
foreach ($column in $requiredColumns) {
    if ($column -notin $rawRows[0].PSObject.Properties.Name) {
        throw "Benchmark input is missing the '$column' column."
    }
}

$rows = foreach ($row in $rawRows) {
    [pscustomobject]@{
        Family = [string]$row.family
        N = [int]$row.n
        K = [int]$row.k
        InputSeed = [uint32]$row.input_seed
        Algorithm = [string]$row.algorithm
        Repetition = [int]$row.repetition
        TimeNs = [double]$row.time_ns
        Result = [int]$row.result
    }
}

$algorithms = @($rows.Algorithm | Sort-Object -Unique)
if (($algorithms -join ",") -ne "median-of-medians,randomised-quickselect") {
    throw "Expected exactly the Median of Medians and Randomized Quickselect rows."
}

$caseGroups = @($rows | Group-Object Family, N, K, InputSeed)
$caseSummaries = foreach ($caseGroup in $caseGroups) {
    $caseRows = @($caseGroup.Group)
    $caseAlgorithms = @($caseRows.Algorithm | Sort-Object -Unique)
    if ($caseAlgorithms.Count -ne 2) {
        throw "A benchmark case does not contain both algorithms: $($caseGroup.Name)"
    }
    if (@($caseRows.Result | Sort-Object -Unique).Count -ne 1) {
        throw "The algorithms returned different results for case: $($caseGroup.Name)"
    }

    foreach ($algorithmGroup in @($caseRows | Group-Object Algorithm)) {
        $algorithmRows = @($algorithmGroup.Group)
        $repetitions = @($algorithmRows.Repetition | Sort-Object)
        if (($repetitions -join ",") -ne "0,1,2,3,4,5,6") {
            throw "Expected repetitions 0 to 6 for $($caseGroup.Name), $($algorithmGroup.Name)."
        }

        $times = [double[]]$algorithmRows.TimeNs
        $median = Get-Median $times
        $minimum = ($times | Measure-Object -Minimum).Minimum
        $maximum = ($times | Measure-Object -Maximum).Maximum
        [pscustomobject]@{
            Family = $caseRows[0].Family
            N = $caseRows[0].N
            K = $caseRows[0].K
            Rank = Get-RankLabel $caseRows[0].N $caseRows[0].K
            Algorithm = $algorithmGroup.Name
            MedianNs = [long][math]::Round($median)
            MinNs = [long][math]::Round($minimum)
            MaxNs = [long][math]::Round($maximum)
            SpreadPercent = [math]::Round(100.0 * ($maximum - $minimum) / $median, 2)
            Result = $caseRows[0].Result
        }
    }
}

$caseSummaries = @($caseSummaries | Sort-Object Family, N, K, Algorithm)
$sizeSummaries = @($caseSummaries | Group-Object Family, N, Algorithm | ForEach-Object {
    $groupRows = @($_.Group)
    [pscustomobject]@{
        Family = $groupRows[0].Family
        N = $groupRows[0].N
        Algorithm = $groupRows[0].Algorithm
        MedianOfRanksNs = [long][math]::Round((Get-Median ([double[]]$groupRows.MedianNs)))
    }
} | Sort-Object Family, N, Algorithm)

$speedupSummaries = @($caseSummaries | Group-Object Family, N, K | ForEach-Object {
    $groupRows = @($_.Group)
    $deterministic = $groupRows | Where-Object Algorithm -eq "median-of-medians"
    $randomised = $groupRows | Where-Object Algorithm -eq "randomised-quickselect"
    if ($null -eq $deterministic -or $null -eq $randomised) {
        throw "Cannot compare algorithms for case: $($_.Name)"
    }
    [pscustomobject]@{
        Family = $groupRows[0].Family
        N = $groupRows[0].N
        K = $groupRows[0].K
        Rank = $groupRows[0].Rank
        MedianOfMediansNs = $deterministic.MedianNs
        RandomisedQuickselectNs = $randomised.MedianNs
        QuickselectSpeedup = [math]::Round($deterministic.MedianNs / $randomised.MedianNs, 3)
    }
} | Sort-Object Family, N, K)

New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$caseSummaryPath = Join-Path $OutputDirectory "benchmark_case_summary.csv"
$sizeSummaryPath = Join-Path $OutputDirectory "benchmark_size_summary.csv"
$speedupSummaryPath = Join-Path $OutputDirectory "benchmark_speedup_summary.csv"
$caseSummaries | Export-Csv -LiteralPath $caseSummaryPath -NoTypeInformation -Encoding utf8
$sizeSummaries | Export-Csv -LiteralPath $sizeSummaryPath -NoTypeInformation -Encoding utf8
$speedupSummaries | Export-Csv -LiteralPath $speedupSummaryPath -NoTypeInformation -Encoding utf8

$familyOrder = @(
    "random-unique", "random-duplicates", "sorted",
    "reverse-sorted", "nearly-sorted", "all-equal"
)
$colours = @{
    "median-of-medians" = "#D55E00"
    "randomised-quickselect" = "#0072B2"
}
$algorithmLabels = @{
    "median-of-medians" = "Median of Medians"
    "randomised-quickselect" = "Randomized Quickselect"
}

# Small multiples keep the six input families readable on one page.
$timeSvg = Start-Svg 1200 800 "Median time by input size"
Add-SvgLine $timeSvg 790 54 825 54 $colours["median-of-medians"] 3
Add-SvgText $timeSvg 833 58 $algorithmLabels["median-of-medians"] 12
Add-SvgLine $timeSvg 990 54 1025 54 $colours["randomised-quickselect"] 3
Add-SvgText $timeSvg 1033 58 $algorithmLabels["randomised-quickselect"] 12

$panelWidth = 340
$panelHeight = 280
$startX = 70
$startY = 90
$gapX = 45
$gapY = 65
$plotPaddingLeft = 58
$plotPaddingTop = 34
$plotWidth = 265
$plotHeight = 195

for ($familyIndex = 0; $familyIndex -lt $familyOrder.Count; ++$familyIndex) {
    $family = $familyOrder[$familyIndex]
    $column = $familyIndex % 3
    $rowIndex = [int][math]::Floor($familyIndex / 3)
    $panelX = $startX + $column * ($panelWidth + $gapX)
    $panelY = $startY + $rowIndex * ($panelHeight + $gapY)
    $plotX = $panelX + $plotPaddingLeft
    $plotY = $panelY + $plotPaddingTop
    $familyRows = @($sizeSummaries | Where-Object Family -eq $family)
    $sizes = @($familyRows.N | Sort-Object -Unique)
    $times = [double[]]$familyRows.MedianOfRanksNs
    $logXMin = [math]::Log10(($sizes | Measure-Object -Minimum).Minimum)
    $logXMax = [math]::Log10(($sizes | Measure-Object -Maximum).Maximum)
    $logYMin = [math]::Log10(($times | Measure-Object -Minimum).Minimum * 0.8)
    $logYMax = [math]::Log10(($times | Measure-Object -Maximum).Maximum * 1.25)

    Add-SvgText $timeSvg ($panelX + $panelWidth / 2) ($panelY + 18) $family 14 "middle" "bold"
    Add-SvgLine $timeSvg $plotX $plotY $plotX ($plotY + $plotHeight) "#333333" 1
    Add-SvgLine $timeSvg $plotX ($plotY + $plotHeight) ($plotX + $plotWidth) ($plotY + $plotHeight) "#333333" 1

    foreach ($tickSize in @(1000, 8000, 64000, 1000000)) {
        $x = $plotX + ([math]::Log10($tickSize) - $logXMin) / ($logXMax - $logXMin) * $plotWidth
        Add-SvgLine $timeSvg $x ($plotY + $plotHeight) $x ($plotY + $plotHeight + 5) "#333333" 1
        Add-SvgText $timeSvg $x ($plotY + $plotHeight + 20) (Get-SizeLabel $tickSize) 10 "middle"
    }

    foreach ($fraction in @(0.0, 0.5, 1.0)) {
        $logValue = $logYMin + $fraction * ($logYMax - $logYMin)
        $y = $plotY + $plotHeight - $fraction * $plotHeight
        Add-SvgLine $timeSvg $plotX $y ($plotX + $plotWidth) $y "#dddddd" 1
        Add-SvgText $timeSvg ($plotX - 7) ($y + 4) (Get-TimeLabel ([math]::Pow(10, $logValue))) 10 "end"
    }

    foreach ($algorithm in @("median-of-medians", "randomised-quickselect")) {
        $algorithmRows = @($familyRows | Where-Object Algorithm -eq $algorithm | Sort-Object N)
        $points = @()
        foreach ($dataRow in $algorithmRows) {
            $x = $plotX + ([math]::Log10($dataRow.N) - $logXMin) / ($logXMax - $logXMin) * $plotWidth
            $y = $plotY + $plotHeight - ([math]::Log10($dataRow.MedianOfRanksNs) - $logYMin) / ($logYMax - $logYMin) * $plotHeight
            $points += [pscustomobject]@{ X = $x; Y = $y }
        }
        for ($pointIndex = 1; $pointIndex -lt $points.Count; ++$pointIndex) {
            Add-SvgLine $timeSvg $points[$pointIndex - 1].X $points[$pointIndex - 1].Y $points[$pointIndex].X $points[$pointIndex].Y $colours[$algorithm] 2.5
        }
        foreach ($point in $points) {
            [void]$timeSvg.AppendLine(('<circle cx="{0:F1}" cy="{1:F1}" r="2.7" fill="{2}" />' -f $point.X, $point.Y, $colours[$algorithm]))
        }
    }
}
Add-SvgText $timeSvg 600 785 "Input size (log scale). Each point is the median of the five rank medians." 12 "middle"
Save-Svg $timeSvg (Join-Path $OutputDirectory "median_time_by_size.svg")

$largestSize = ($speedupSummaries.N | Measure-Object -Maximum).Maximum
$largestSpeedups = foreach ($family in $familyOrder) {
    $familyRows = @($speedupSummaries | Where-Object { $_.Family -eq $family -and $_.N -eq $largestSize })
    [pscustomobject]@{
        Family = $family
        Speedup = Get-Median ([double[]]$familyRows.QuickselectSpeedup)
    }
}
$speedupSvg = Start-Svg 1000 520 "Randomized Quickselect speedup at n = 1,000,000"
$chartX = 220
$chartY = 75
$chartWidth = 700
$barHeight = 42
$barGap = 20
$maximumSpeedup = (($largestSpeedups.Speedup | Measure-Object -Maximum).Maximum * 1.12)
$baselineX = $chartX + (1.0 / $maximumSpeedup) * $chartWidth
Add-SvgLine $speedupSvg $baselineX ($chartY - 12) $baselineX 460 "#555555" 1.5
Add-SvgText $speedupSvg $baselineX 482 "1x" 11 "middle"
for ($index = 0; $index -lt $largestSpeedups.Count; ++$index) {
    $item = $largestSpeedups[$index]
    $y = $chartY + $index * ($barHeight + $barGap)
    $width = $item.Speedup / $maximumSpeedup * $chartWidth
    Add-SvgText $speedupSvg ($chartX - 15) ($y + 27) $item.Family 13 "end"
    [void]$speedupSvg.AppendLine(('<rect x="{0:F1}" y="{1:F1}" width="{2:F1}" height="{3}" fill="#0072B2" />' -f $chartX, $y, $width, $barHeight))
    Add-SvgText $speedupSvg ($chartX + $width + 8) ($y + 27) ("{0:F2}x" -f $item.Speedup) 13
}
Add-SvgText $speedupSvg 500 508 "Median ratio across the five requested ranks. Higher means Quickselect was faster." 12 "middle"
Save-Svg $speedupSvg (Join-Path $OutputDirectory "speedup_at_largest_size.svg")

$rankSvg = Start-Svg 1200 800 "Rank effect at n = 1,000,000"
Add-SvgLine $rankSvg 790 54 825 54 $colours["median-of-medians"] 3
Add-SvgText $rankSvg 833 58 $algorithmLabels["median-of-medians"] 12
Add-SvgLine $rankSvg 990 54 1025 54 $colours["randomised-quickselect"] 3
Add-SvgText $rankSvg 1033 58 $algorithmLabels["randomised-quickselect"] 12
$rankOrder = @("minimum", "quarter", "median", "three-quarter", "maximum")
$rankLabels = @("min", "25%", "50%", "75%", "max")
for ($familyIndex = 0; $familyIndex -lt $familyOrder.Count; ++$familyIndex) {
    $family = $familyOrder[$familyIndex]
    $column = $familyIndex % 3
    $rowIndex = [int][math]::Floor($familyIndex / 3)
    $panelX = $startX + $column * ($panelWidth + $gapX)
    $panelY = $startY + $rowIndex * ($panelHeight + $gapY)
    $plotX = $panelX + $plotPaddingLeft
    $plotY = $panelY + $plotPaddingTop
    $familyRows = @($caseSummaries | Where-Object { $_.Family -eq $family -and $_.N -eq $largestSize })
    $maximumTime = ($familyRows.MedianNs | Measure-Object -Maximum).Maximum * 1.12
    Add-SvgText $rankSvg ($panelX + $panelWidth / 2) ($panelY + 18) $family 14 "middle" "bold"
    Add-SvgLine $rankSvg $plotX $plotY $plotX ($plotY + $plotHeight) "#333333" 1
    Add-SvgLine $rankSvg $plotX ($plotY + $plotHeight) ($plotX + $plotWidth) ($plotY + $plotHeight) "#333333" 1
    foreach ($fraction in @(0.0, 0.5, 1.0)) {
        $y = $plotY + $plotHeight - $fraction * $plotHeight
        Add-SvgLine $rankSvg $plotX $y ($plotX + $plotWidth) $y "#dddddd" 1
        Add-SvgText $rankSvg ($plotX - 7) ($y + 4) (Get-TimeLabel ($maximumTime * $fraction)) 10 "end"
    }
    for ($rankIndex = 0; $rankIndex -lt $rankOrder.Count; ++$rankIndex) {
        $x = $plotX + $rankIndex / ($rankOrder.Count - 1) * $plotWidth
        Add-SvgText $rankSvg $x ($plotY + $plotHeight + 20) $rankLabels[$rankIndex] 10 "middle"
    }
    foreach ($algorithm in @("median-of-medians", "randomised-quickselect")) {
        $points = @()
        for ($rankIndex = 0; $rankIndex -lt $rankOrder.Count; ++$rankIndex) {
            $dataRow = $familyRows | Where-Object { $_.Algorithm -eq $algorithm -and $_.Rank -eq $rankOrder[$rankIndex] }
            $x = $plotX + $rankIndex / ($rankOrder.Count - 1) * $plotWidth
            $y = $plotY + $plotHeight - $dataRow.MedianNs / $maximumTime * $plotHeight
            $points += [pscustomobject]@{ X = $x; Y = $y }
        }
        for ($pointIndex = 1; $pointIndex -lt $points.Count; ++$pointIndex) {
            Add-SvgLine $rankSvg $points[$pointIndex - 1].X $points[$pointIndex - 1].Y $points[$pointIndex].X $points[$pointIndex].Y $colours[$algorithm] 2.5
        }
        foreach ($point in $points) {
            [void]$rankSvg.AppendLine(('<circle cx="{0:F1}" cy="{1:F1}" r="3" fill="{2}" />' -f $point.X, $point.Y, $colours[$algorithm]))
        }
    }
}
Add-SvgText $rankSvg 600 785 "Requested rank. Each point is the median of seven timed repetitions." 12 "middle"
Save-Svg $rankSvg (Join-Path $OutputDirectory "rank_effect_at_largest_size.svg")

Write-Host "Summarised $($caseGroups.Count) cases from $($rows.Count) timing rows."
Write-Host "Analysis files written to $OutputDirectory"
