# Generic PowerShell build script for the Quant Risk Platform
# This script builds both C++ and Python components

param(
    [string]$BuildType = "Release",
    [string[]]$ExtraArgs = @()
)

$scriptPath = $MyInvocation.MyCommand.Path
$projectRoot = Split-Path (Split-Path $scriptPath -Parent) -Parent
$buildDir = "$projectRoot\build\$BuildType"

Write-Host "--- Starting Build for Quant Risk Platform ($BuildType) ---" -ForegroundColor Cyan

# Note: find_package(Python3) in CMakeLists.txt handles discovery.

# 1. Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Yellow

$cmakeArgs = @("-S", $projectRoot, "-B", $buildDir, "-DCMAKE_BUILD_TYPE=$BuildType")
$cmakeArgs += $ExtraArgs

# Practical discovery order:
# 1. Explicit CMake inputs (Python3_EXECUTABLE, etc. - passed via ExtraArgs)
# 2. VCPKG_ROOT env var for toolchain location
# 3. Normal discovery from PATH

# vcpkg toolchain discovery:
# CMAKE_TOOLCHAIN_FILE is the canonical way to point to vcpkg.
# If VCPKG_ROOT is set, it is used to derive the toolchain location.
if ($env:VCPKG_ROOT) {
    $vcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"
    if (Test-Path $vcpkgToolchain) {
        $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
        Write-Host "Using vcpkg toolchain derived from VCPKG_ROOT: $vcpkgToolchain" -ForegroundColor Gray
    } else {
        Write-Error "VCPKG_ROOT is set but toolchain file not found at: $vcpkgToolchain"
        exit 1
    }
}

# Python discovery:
# This project relies on standard FindPython3 behavior.
# Users should pass -Python3_EXECUTABLE=... or -Python3_ROOT_DIR=... as ExtraArgs
# if they wish to override discovery. No environment variables are used.

cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

# 2. Build the project
Write-Host "Building all targets..." -ForegroundColor Yellow
cmake --build $buildDir --config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed. See errors above." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "--- Build Successful! ---" -ForegroundColor Green
Write-Host "Artifacts are in: $buildDir"
