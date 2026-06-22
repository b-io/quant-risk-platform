# Generic PowerShell build script for the Quant Risk Platform
# This script builds both C++ and Python components

param(
    [string]$Preset = "Release-Python",
    [string[]]$ExtraArgs = @(),
    [switch]$SkipEnv
)

$scriptPath = $MyInvocation.MyCommand.Path
$projectRoot = Split-Path (Split-Path $scriptPath -Parent) -Parent

Write-Host "--- Starting Build for Quant Risk Platform (Preset: $Preset) ---" -ForegroundColor Cyan

if (-not $SkipEnv) {
    $envScript = Join-Path $projectRoot "scripts\env.ps1"
    if (Test-Path -LiteralPath $envScript) {
        & $envScript -ProjectRoot $projectRoot -Quiet
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
}

# 1. Configure CMake
Write-Host "Configuring CMake with preset..." -ForegroundColor Yellow

$cmakeArgs = @("--preset", $Preset)
$cmakeArgs += $ExtraArgs

# Note: Presets in CMakePresets.json already handle:
# - CMAKE_TOOLCHAIN_FILE (via $env:VCPKG_ROOT)
# - VCPKG_INSTALL_OPTIONS (--allow-unsupported)
# - BUILD_TESTING
# - QRP_BUILD_PYTHON

cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

# 2. Build the project
Write-Host "Building with preset..." -ForegroundColor Yellow
cmake --build --preset $Preset

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed. See errors above." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "--- Build Successful! ---" -ForegroundColor Green
