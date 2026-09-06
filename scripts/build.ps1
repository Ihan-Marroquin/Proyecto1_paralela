param(
    [ValidateSet("release", "debug")]
    [string]$Configuration = "release"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $projectRoot "build"
$outputFile = Join-Path $outputDirectory "aurora_saver.exe"

$compilerCommand = Get-Command "g++" -ErrorAction SilentlyContinue
if (-not $compilerCommand) {
    $msysCompiler = "C:\msys64\ucrt64\bin\g++.exe"
    if (Test-Path -LiteralPath $msysCompiler) {
        $compilerPath = $msysCompiler
    } else {
        throw "No se encontro g++. Instale MSYS2 UCRT64 y agregue su carpeta bin al PATH."
    }
} else {
    $compilerPath = $compilerCommand.Source
}

New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$sourceFiles = @(
    "src/main.cpp",
    "src/AppConfig.cpp",
    "src/AuroraSimulation.cpp",
    "src/AuroraRenderer.cpp",
    "src/Benchmark.cpp",
    "src/ScreensaverWindow.cpp"
) | ForEach-Object { Join-Path $projectRoot $_ }

$compilerArguments = @(
    "-std=c++17",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-fopenmp",
    "-DNOMINMAX",
    "-DWIN32_LEAN_AND_MEAN",
    "-I$(Join-Path $projectRoot 'include')"
)

if ($Configuration -eq "debug") {
    $compilerArguments += @("-O0", "-g")
} else {
    $compilerArguments += @("-O2", "-DNDEBUG")
}

$compilerArguments += $sourceFiles
$compilerArguments += @("-lgdi32", "-luser32", "-o", $outputFile)

& $compilerPath @compilerArguments
if ($LASTEXITCODE -ne 0) {
    throw "La compilacion termino con codigo $LASTEXITCODE."
}

Write-Host "Compilacion completada: $outputFile"

