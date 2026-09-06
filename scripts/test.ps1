$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot "build.ps1") -Configuration release
$executable = Join-Path $projectRoot "build\aurora_saver.exe"

& $executable 8 --self-test --threads 4 --width 640 --height 480
if ($LASTEXITCODE -ne 0) {
    throw "Las pruebas internas fallaron."
}

function Test-InvalidArguments {
    param([string[]]$Arguments)

    & $executable @Arguments *> $null
    if ($LASTEXITCODE -eq 0) {
        throw "Se aceptaron argumentos que debian rechazarse: $($Arguments -join ' ')"
    }
}

Test-InvalidArguments @("--headless")
Test-InvalidArguments @("8", "--width", "639", "--headless")
Test-InvalidArguments @("8", "--mode", "rapido", "--headless")
Test-InvalidArguments @("8", "--benchmark", "--trials", "9")
Test-InvalidArguments @("8", "9", "--headless")

& $executable --help *> $null
if ($LASTEXITCODE -ne 0) {
    throw "La ayuda deberia terminar correctamente."
}

Write-Host "Validacion de argumentos completada correctamente."

