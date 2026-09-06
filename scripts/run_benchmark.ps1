param(
    [ValidateRange(1, 512)]
    [int]$Sources = 16,

    [ValidateRange(1, 256)]
    [int]$Threads = [Environment]::ProcessorCount,

    [ValidateRange(10, 1000)]
    [int]$Trials = 10,

    [ValidateRange(640, 3840)]
    [int]$Width = 960,

    [ValidateRange(480, 2160)]
    [int]$Height = 540
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot "build.ps1") -Configuration release

$executable = Join-Path $projectRoot "build\aurora_saver.exe"
$csvPath = Join-Path $projectRoot "results\benchmark.csv"
& $executable $Sources --benchmark --threads $Threads --trials $Trials `
    --width $Width --height $Height --csv $csvPath

if ($LASTEXITCODE -ne 0) {
    throw "El benchmark termino con codigo $LASTEXITCODE."
}

