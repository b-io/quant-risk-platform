# Quickstart Guide

This guide explains supported build workflows on Windows using CMake and how to enable optional Python bindings safely.

## Supported configurations

- Recommended daily profile: `dev`, which builds C++ plus Python with a static `qrp_core`.
- Shared-core validation profile: `dev-shared`, which builds C++ plus Python with a shared `qrp_core`.
- Production-style profiles: `release` and `release-shared`.
- Native C++ debugging profiles: `debug` and `debug-shared`.
- Python bindings on Windows in Debug require a matching debug CPython and are not supported by default.

## 1. Build the C++ core (no Python)

- Using CLion profiles (preferred in this repo):
    - `Debug - C++ only (static core)` for native C++ debugging.
    - `Debug - C++ only (shared core)` for native C++ debugging with a shared `qrp_core`.
- From command line with presets:
    - Configure: `cmake --preset debug`
    - Build: `cmake --build build/debug --target qrp_core`

## 2. Build C++ plus Python

- Recommended presets:
    - `cmake --preset dev`
    - `cmake --build build/dev --target quant_risk_platform`
    - Smoke test: `ctest --test-dir build/dev -R python_import`
- In CLion, enable the `Dev - C++ + Python (static core)` profile for day-to-day work and
  `Dev - C++ + Python (shared core)` when validating shared-core behavior.

## 3. Build and Test With Automation Scripts

The easiest way to build and verify the platform is to use the canonical scripts.

### On Windows (PowerShell):

```powershell
# Build
.\scripts\build.ps1

# Build and run all tests
.\scripts\test.ps1

# Build and run tests plus the C++ portfolio benchmark
.\scripts\test.ps1 -Performance

# Install Python/package dependencies
.\scripts\install.ps1
```

### On Linux / WSL (Bash):

```bash
# Build and run all tests
chmod +x scripts/*.sh
./scripts/build.sh
./scripts/test.sh

# Build and run tests plus the C++ portfolio benchmark
./scripts/test.sh -Performance

# Install Python/package dependencies
./scripts/install.sh
```

## 4. Coverage

The canonical local coverage workflow is the PowerShell test script:

```powershell
.\scripts\test.ps1 -Coverage
```

On Windows with MSVC this uses the native Visual Studio coverage collector when available. The Bash script uses the
CMake/gcovr coverage target for GCC/Clang builds. The repository coverage gate is 95% line coverage for both C++ and
Python. MSVC coverage runs the full unit test executable by default. To narrow the instrumented GoogleTest set while
debugging coverage collector issues, set `QRP_MSVC_COVERAGE_GTEST_FILTER` explicitly.

The committed summary artifacts are refreshed under `coverage/`:

- `coverage/coverage-badge.svg`
- `coverage/coverage_summary.json`
- `coverage/coverage_summary.md`

GCC/Clang-style coverage remains available through the CMake coverage preset:

```bash
cmake --preset coverage
cmake --build build/coverage --target coverage
```

## 5. Run a Demo

After building, you can run a Python demo or use the C++ CLI.

### Python Demo

```powershell
# Build the extension first, then run the pinned Python 3.12 demo environment.
cmake --preset dev
cmake --build build/dev --target quant_risk_platform
uv sync --project python --extra dashboard --extra optimization
uv run --project python python python\examples\demo_platform.py
```

### C++ CLI

```powershell
# Adjust the path to your build directory if you use a preset other than dev.
$cli = ".\build\dev\cpp\cli\qrp_cli.exe"

& $cli init-db
& $cli import-market data\market\demo_market.json
& $cli import-portfolio data\portfolios\demo_portfolio.json
& $cli import-scenarios data\scenarios\demo_scenarios.json

& $cli run-valuation --portfolio demo_portfolio --snapshot DEMO_MKT_2026_03_24
& $cli run-risk --portfolio demo_portfolio --snapshot DEMO_MKT_2026_03_24
& $cli run-pnl-explain --portfolio demo_portfolio --previous-snapshot DEMO_MKT_2026_03_24 --snapshot DEMO_MKT_2026_03_24
& $cli run-hvar --portfolio demo_portfolio --snapshot DEMO_MKT_2026_03_24 --scenarios demo_factor_scenarios
& $cli list
```

## 6. Artifacts and Outputs

- **Database**: Persistent data is stored in `var/quant_risk_platform.sqlite` by default.
- **Logs**: Application logs are written to `var/logs/`.
- **Binaries**: Built executables (`qrp_cli`, `unit_tests`, `integration_tests`) are located in the build directory
  under `cpp/cli/` and `tests/`.

---

## Advanced: Manual Build (CMake)

If you need to customize the build, you can use standard CMake commands.

### Prerequisites

The platform depends on:

- **C++ Compiler**: Supporting C++20 (MSVC 2022 recommended).
- **vcpkg**: For managing C++ dependencies. Ensure the environment variable `VCPKG_ROOT` is set to your local vcpkg
  installation path.

### Environment Setup

To ensure CMake finds vcpkg automatically via presets and you can use `vcpkg` from the command line, set the
`VCPKG_ROOT` environment variable and add it to your `PATH`:

```powershell
# Set these for the current session
$env:DEV_TOOLS_ROOT = "<path-to-your-dev-tools>"
$env:VCPKG_ROOT = "$env:DEV_TOOLS_ROOT\vcpkg"
# Add to PATH for current session
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"

# Set permanently for user (recommended)
[Environment]::SetEnvironmentVariable("DEV_TOOLS_ROOT", "<path-to-your-dev-tools>", "User")
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "$env:DEV_TOOLS_ROOT\vcpkg", "User")
$oldPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($oldPath -notlike "*$env:VCPKG_ROOT*") {
    [Environment]::SetEnvironmentVariable("Path", "$oldPath;$env:VCPKG_ROOT", "User")
}
```

The CMake presets read `VCPKG_TARGET_TRIPLET` from the environment. Use one of the tracked templates below to set the
right values for your platform.

When the environment file is active, CMake prints an early `Environment` banner before vcpkg starts, including the
`QRP_ENV_PROFILE`, `VCPKG_ROOT`, and `VCPKG_TARGET_TRIPLET` values.

#### Windows with MSVC

If you use CLion with the MSVC toolchain from Visual Studio or Build Tools, CLion may inherit Visual Studio's bundled
vcpkg instead of your regular vcpkg installation. In that case, use a local environment script:

1. Create `.env.cmd` from `msvc.env.cmd.example`.
2. Edit `DEV_TOOLS_ROOT` for your machine-local tool directory.
3. In CLion, open `Settings > Build, Execution, Deployment > Toolchains`.
4. Select the MSVC/Visual Studio toolchain.
5. Set `Environment file` to the project-local `.env.cmd`.
6. Reset the CMake cache and reload the project.

Example `.env.cmd`:

```cmd
@echo off
set "DEV_TOOLS_ROOT=%USERPROFILE%\dev-tools"
set "QRP_VS_INSTALL_DIR=%VSINSTALLDIR%"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not defined QRP_VS_INSTALL_DIR (
    if exist "%VSWHERE%" (
        for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "QRP_VS_INSTALL_DIR=%%I"
    )
)

if not defined QRP_VS_INSTALL_DIR (
    for %%V in (
        "%ProgramFiles%\Microsoft Visual Studio\18\BuildTools"
        "%ProgramFiles%\Microsoft Visual Studio\18\Community"
        "%ProgramFiles%\Microsoft Visual Studio\18\Enterprise"
        "%ProgramFiles%\Microsoft Visual Studio\18\Professional"
        "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Community"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Professional"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional"
    ) do (
        if exist "%%~V\Common7\Tools\VsDevCmd.bat" set "QRP_VS_INSTALL_DIR=%%~V"
    )
)

if defined QRP_VS_INSTALL_DIR if exist "%QRP_VS_INSTALL_DIR%\Common7\Tools\VsDevCmd.bat" (
    call "%QRP_VS_INSTALL_DIR%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
)

set "QRP_MSVC_ENV_READY=OFF"
if defined INCLUDE if defined LIB set "QRP_MSVC_ENV_READY=ON"

set "CC=cl"
set "CXX=cl"
set "QRP_ENV_FILE=%~f0"
set "QRP_ENV_PROFILE=msvc"
rem Optional: set this if Microsoft.CodeCoverage.Console.exe is not on PATH.
rem set "QRP_VS_COVERAGE_TOOL=%QRP_VS_INSTALL_DIR%\Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe"
set "VCPKG_ROOT=%DEV_TOOLS_ROOT%\vcpkg"
set "VCPKG_TARGET_TRIPLET=x64-windows-static-md"
set "PATH=%DEV_TOOLS_ROOT%;%DEV_TOOLS_ROOT%\ninja;%VCPKG_ROOT%;%PATH%"
```

The real `.env.cmd` is ignored by Git because it contains machine-local paths. The tracked `msvc.env.cmd.example` is
only a
template.
The `VsDevCmd.bat` initialization is important for MSVC builds because vcpkg host tools use `link.exe` during configure
checks and need Visual Studio's `LIB`, `INCLUDE`, and `PATH` environment variables.

If you build from a normal PowerShell terminal instead of CLion, import the same environment before calling CMake. The
PowerShell build, install, and test scripts do this automatically unless you pass `-SkipEnv`.

```powershell
. .\scripts\env.ps1
cmake --build build\dev --target unit_tests -j 10
```

Without this step, Ninja can still find `cl.exe` from the generated build files, but `cl.exe` may not find MSVC standard
headers such as `<string>` or `<cstddef>` because `INCLUDE` and `LIB` were never loaded into the shell.

The Windows template uses the vcpkg triplet `x64-windows-static-md`. This keeps third-party libraries statically linked,
which is required by the vcpkg QuantLib port on Windows, while still using the dynamic MSVC runtime expected by Python
extension builds.

#### Linux with GCC

Create a local shell environment file from `gcc.env.sh.example`:

```bash
cp gcc.env.sh.example .env.sh
```

Edit `DEV_TOOLS_ROOT` or `VCPKG_ROOT` inside `.env.sh` if needed, then source it before configuring:

```bash
. ./scripts/env.sh
cmake --preset dev
cmake --build build/dev
```

The GCC template uses `gcc` and `g++`, and chooses `x64-linux` or `arm64-linux` based on `uname`.

#### Linux or macOS with Clang

Create a local shell environment file from `clang.env.sh.example`:

```bash
cp clang.env.sh.example .env.sh
```

Edit `DEV_TOOLS_ROOT` or `VCPKG_ROOT` inside `.env.sh` if needed, then source it before configuring:

```bash
. ./scripts/env.sh
cmake --preset dev
cmake --build build/dev
```

The Clang template uses `clang` and `clang++`, and chooses `x64-linux`, `arm64-linux`, `x64-osx`, or `arm64-osx` based
on `uname`. The real `.env.sh` is ignored by Git because it contains machine-local paths.

The Bash build, install, and test scripts source `scripts/env.sh` automatically when `.env.sh` exists. Pass
`-SkipEnv` if you intentionally want to use the current shell environment as-is.

### Build Steps

```powershell
# Configure the project using presets (uses $env:VCPKG_ROOT automatically)
cmake --preset dev

# Build
cmake --build build/dev
```

---

## Full Prerequisites List

### C++ Dependencies

- **fmt**: Formatting library.
- **GoogleTest**: Testing framework.
- **nlohmann-json**: JSON for Modern C++.
- **pybind11**: C++11 bindings for Python.
- **QuantLib**: Financial instrument and pricing library.

### Tools

- **CMake**: 3.21 or higher.
- **Python**: 3.9 or higher.
