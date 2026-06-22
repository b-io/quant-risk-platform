[CmdletBinding()]
param(
    [string]$EnvFile,
    [string]$ProjectRoot,
    [switch]$Quiet
)

# Import the project-local .env.cmd into the current PowerShell process.
# This keeps CLion and terminal builds on the same MSVC/vcpkg environment.

if (-not $ProjectRoot) {
    if ($PSScriptRoot) {
        $ProjectRoot = Split-Path $PSScriptRoot -Parent
    } else {
        $ProjectRoot = (Get-Location).Path
    }
}

if (-not $EnvFile) {
    $EnvFile = Join-Path $ProjectRoot ".env.cmd"
}

$isWindows = [System.Environment]::OSVersion.Platform -eq [System.PlatformID]::Win32NT
if (-not $isWindows) {
    if (-not $Quiet) {
        Write-Host "Skipping .env.cmd import on non-Windows platform." -ForegroundColor Yellow
    }
    return
}

if (-not (Test-Path -LiteralPath $EnvFile)) {
    if (-not $Quiet) {
        Write-Host "No environment file found at $EnvFile; using current process environment." -ForegroundColor Yellow
    }
    return
}

$cmd = "`"$EnvFile`" >nul && set"
$envOutput = & $env:ComSpec /d /s /c $cmd
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to import environment from $EnvFile"
    exit $LASTEXITCODE
}

foreach ($line in $envOutput) {
    if ($line -match "^(.*?)=(.*)$") {
        $name = $Matches[1]
        $value = $Matches[2]
        if ($name.Length -eq 0 -or $name.StartsWith("=")) {
            continue
        }
        [System.Environment]::SetEnvironmentVariable($name, $value, "Process")
    }
}

if (-not $Quiet) {
    Write-Host "Imported environment from $EnvFile" -ForegroundColor Green
    Write-Host "MSVC env ready       : $env:QRP_MSVC_ENV_READY"
    Write-Host "QRP env profile      : $env:QRP_ENV_PROFILE"
    Write-Host "VCPKG_ROOT           : $env:VCPKG_ROOT"
    Write-Host "VCPKG_TARGET_TRIPLET : $env:VCPKG_TARGET_TRIPLET"
}

if (-not $env:INCLUDE -or -not $env:LIB) {
    Write-Warning "MSVC INCLUDE/LIB were not detected. Ninja builds with cl.exe may fail to find standard headers such as <string> or <cstddef>."
}
