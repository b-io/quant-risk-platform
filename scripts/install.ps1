# Install dependencies for Quant Risk Platform

# 1. Ensure Python dependencies
Write-Host "--- Selecting Python Interpreter ---" -ForegroundColor Cyan

$projectRoot = $PSScriptRoot
if (!$projectRoot) { $projectRoot = Get-Location }
else { $projectRoot = Split-Path $projectRoot -Parent }

# Find the vcpkg-provided Python 3.12 if it exists
$vcpkgPython = $null
$candidates = @(
    "build\Release-Python\vcpkg_installed\x64-windows\tools\python3\python.exe",
    "build\Release\vcpkg_installed\x64-windows\tools\python3\python.exe",
    "build\Debug\vcpkg_installed\x64-windows\tools\python3\python.exe"
)

foreach ($relPath in $candidates) {
    $fullPath = Join-Path $projectRoot $relPath
    if (Test-Path $fullPath) {
        $vcpkgPython = $fullPath
        break
    }
}

$pythonExec = "python"
if ($vcpkgPython) {
    $pythonExec = $vcpkgPython
    Write-Host "Using vcpkg-provided Python: $pythonExec" -ForegroundColor Cyan
    
    # Check if pip is available, if not, try to ensure it
    & $pythonExec -m pip --version 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "pip not found for vcpkg Python. Attempting to install pip..." -ForegroundColor Yellow
        & $pythonExec -m ensurepip --upgrade
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Failed to install pip for vcpkg Python. Falling back to system 'python'."
            $pythonExec = "python"
        }
    }
} else {
    Write-Warning "vcpkg Python not found. Using system 'python'. This might cause issues if version mismatch exists."
}

Write-Host "--- Installing Python dependencies ---" -ForegroundColor Cyan
if (Test-Path "requirements.txt") {
    & $pythonExec -m pip install -r requirements.txt
}
if (Test-Path "pyproject.toml") {
    & $pythonExec -m pip install -e .
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
    & $vcpkgPath install --allow-unsupported --triplet x64-windows
} else {
    Write-Warning "vcpkg not found in PATH or VCPKG_ROOT. Please install vcpkg or set VCPKG_ROOT."
}

Write-Host "--- Dependency installation complete ---" -ForegroundColor Green
