param(
    [switch]$SelfTest,
    [switch]$Run
)

$ErrorActionPreference = "Stop"
$SimDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Cache = Join-Path $env:LOCALAPPDATA "pixxcraft-sim-tools"
$Gcc = Join-Path $Cache "w64devkit\bin\g++.exe"
$Sdl = Join-Path $Cache "SDL2-2.32.10\x86_64-w64-mingw32"

if (-not (Test-Path $Gcc)) {
    Write-Error "g++ was not found at $Gcc"
}
if (-not (Test-Path (Join-Path $Sdl "lib\libSDL2.dll.a"))) {
    Write-Error "SDL2 was not found at $Sdl"
}

$OutDir = Join-Path $SimDir "build"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$Exe = Join-Path $OutDir "pixxcraft_sim.exe"

$sources = @(
    "main.cpp",
    "log.cpp",
    "gpio.cpp",
    "st7789.cpp",
    "cst816.cpp",
    "arduino_api.cpp",
    "facade.cpp",
    "host.cpp",
    "self_test.cpp",
    "api_export.cpp",
    "loader.cpp"
) | ForEach-Object { Join-Path (Join-Path $SimDir "src") $_ }

& $Gcc -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter `
    "-I$(Join-Path $SimDir 'include')" `
    "-I$(Join-Path $SimDir 'src')" `
    "-I$(Join-Path $Sdl 'include\SDL2')" `
    @sources `
    "-L$(Join-Path $Sdl 'lib')" `
    -lSDL2 -lcomdlg32 -static-libgcc -static-libstdc++ `
    -o $Exe
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Copy-Item (Join-Path $Sdl "bin\SDL2.dll") (Join-Path $OutDir "SDL2.dll") -Force
Write-Host "built $Exe"

if ($SelfTest) {
    & $Exe --self-test --dump (Join-Path $OutDir "selftest.bmp")
    exit $LASTEXITCODE
}
if ($Run) {
    & $Exe
}
