# NEXO Engine - build de un comando para Windows.
# Requisitos: CMake >= 3.24 y GCC de MSYS2 UCRT64 (mingw-w64-ucrt-x86_64-gcc).
# Uso:  .\build.ps1 [-Test] [-Run]
param(
    [switch]$Test,
    [switch]$Run
)

$ErrorActionPreference = "Stop"

# --- Localizar CMake (PATH o instalacion portable de winget) ---
$cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakeCmd) {
    $portable = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter cmake.exe -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($portable) { $cmakeCmd = @{ Source = $portable.FullName } }
}
if (-not $cmakeCmd) { throw "CMake no encontrado. Instalalo con: winget install Kitware.CMake" }
$cmake = $cmakeCmd.Source
$ctest_cmd = Join-Path (Split-Path $cmake -Parent) "ctest.exe"

# --- Localizar el compilador de MSYS2 UCRT64 ---
$ucrt64 = "C:\msys64\ucrt64\bin"
if (Test-Path "$ucrt64\g++.exe") {
    $env:Path = "$ucrt64;$env:Path"
} elseif (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    throw "No hay compilador. Instalalo con MSYS2: pacman -S mingw-w64-ucrt-x86_64-gcc"
}

Write-Host "==> Configurando (Ninja, Release)" -ForegroundColor Cyan
& $cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "CMake configure fallo" }

Write-Host "==> Compilando" -ForegroundColor Cyan
& $cmake --build build
if ($LASTEXITCODE -ne 0) { throw "La compilacion fallo" }

if ($Test) {
    Write-Host "==> Corriendo tests" -ForegroundColor Cyan
    & $ctest_cmd --test-dir build --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Los tests fallaron" }
}

if ($Run) {
    Write-Host "==> Lanzando NexoEngine" -ForegroundColor Cyan
    Start-Process "$PWD\build\NexoEngine.exe" -WorkingDirectory "$PWD\build"
} else {
    Write-Host "Listo: build\NexoEngine.exe  (ejecutalo desde build\ para que encuentre assets y data\)" -ForegroundColor Green
}
