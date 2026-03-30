# Install dependencies for Quant Risk Platform

# 1. Ensure Python dependencies
Write-Host "--- Installing Python dependencies ---" -ForegroundColor Cyan
if (Test-Path "requirements.txt") {
    pip install -r requirements.txt
}
if (Test-Path "pyproject.toml") {
    pip install -e .
}

# 2. Ensure C++ dependencies via vcpkg
Write-Host "--- Checking vcpkg ---" -ForegroundColor Cyan
$vcpkgPath = $null
if ($env:VCPKG_ROOT -and (Test-Path "$env:VCPKG_ROOT\vcpkg.exe")) {
    $vcpkgPath = "$env:VCPKG_ROOT\vcpkg.exe"
} elseif (Get-Command vcpkg -ErrorAction SilentlyContinue) {
    $vcpkgPath = "vcpkg"
}

if ($vcpkgPath) {
    Write-Host "Using vcpkg at: $vcpkgPath"
    & $vcpkgPath install --triplet x64-windows
} else {
    Write-Warning "vcpkg not found in PATH or VCPKG_ROOT. Please install vcpkg or set VCPKG_ROOT."
}

Write-Host "--- Dependency installation complete ---" -ForegroundColor Green
