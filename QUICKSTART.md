# Quickstart Guide

This guide explains supported build workflows on Windows using CMake and how to enable optional Python bindings safely.

## Supported configurations

- C++ only: Debug, Release, RelWithDebInfo
- Python bindings on Windows: Release, RelWithDebInfo (Debug requires a matching debug CPython and is not supported by
  default)

## 1. Build the C++ core (no Python)

- Using CLion profiles (preferred in this repo):
    - Profile "Debug" → C++ only Debug build
    - Profile "Release" → C++ only Release build
- From command line with presets:
    - Configure: `cmake --preset Release`
    - Build: `cmake --build --preset Release --target qrp_core`

## 2. Enable Python bindings (Windows Release/RelWithDebInfo)

- Recommended presets:
    - `cmake --preset Release-Python`
    - `cmake --build --preset Release-Python --target quant_risk_platform`
    - Smoke test: `ctest --preset Release-Python -R python_import`
- Or via CLion "Release" profile by setting `QRP_BUILD_PYTHON=ON` in CMake options.

## 3. Quick Build and Test (automation scripts)

The easiest way to build and verify the platform is to use the canonical scripts.

### On Windows (PowerShell):

```powershell
# Build
.\scripts\build.ps1

# Build and run all tests
.\scripts\test.ps1

# Install Python/package dependencies
.\scripts\install.ps1
```

### On Linux / WSL (Bash):

```bash
# Build and run all tests
chmod +x scripts/*.sh
./scripts/build.sh
./scripts/test.sh

# Install Python/package dependencies
./scripts/install.sh
```

## 4. Coverage

Coverage is supported for GCC/Clang-style toolchains via `gcovr`:

```bash
cmake --preset Coverage
cmake --build --preset Coverage --target coverage
```

The coverage target runs tests and fails below `QRP_COVERAGE_MIN_LINE`, which defaults to `85`.
Use `-DQRP_COVERAGE_MIN_LINE=90` if you want to enforce a 90% line-coverage gate.

## 5. Run a Demo

After building, you can run a Python demo or use the C++ CLI.

### Python Demo

```bash
# Ensure you are in a virtual environment with requirements installed
python -m venv venv
# Windows PowerShell:
.\venv\Scripts\Activate.ps1
# Install runtime deps
pip install -r requirements.txt

# After building the extension (see Section 2), make sure Python can find it.
# If you used presets, the .pyd is placed under build/<preset>/python/<config>
# Example (Release preset):
$Env:PYTHONPATH = "build/Release-Python/python/Release"  # PowerShell
python python/examples/demo_platform.py
```

### C++ CLI

```bash
# Adjust the path to your build directory (e.g., build/Debug or build/Release)
./build/Debug/qrp_cli price --market data/market/demo_market.json --portfolio data/portfolios/demo_portfolio.json
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
- **vcpkg**: For managing C++ dependencies. Ensure the environment variable `VCPKG_ROOT` is set to your vcpkg
  installation path (e.g., `D:\BIN\vcpkg`).

### Environment Setup

To ensure CMake finds vcpkg automatically via presets and you can use `vcpkg` from the command line, set the
`VCPKG_ROOT` environment variable and add it to your `PATH`:

```powershell
# Set VCPKG_ROOT for current session
$env:VCPKG_ROOT = "D:\BIN\vcpkg"
# Add to PATH for current session
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"

# Set permanently for user (recommended)
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "D:\BIN\vcpkg", "User")
$oldPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($oldPath -notlike "*D:\BIN\vcpkg*") {
    [Environment]::SetEnvironmentVariable("Path", "$oldPath;D:\BIN\vcpkg", "User")
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
2. Edit `DEV_TOOLS_ROOT` if your tools are not installed under `D:\BIN`.
3. In CLion, open `Settings > Build, Execution, Deployment > Toolchains`.
4. Select the MSVC/Visual Studio toolchain.
5. Set `Environment file` to the project-local `.env.cmd`.
6. Reset the CMake cache and reload the project.

Example `.env.cmd`:

```cmd
@echo off
set "DEV_TOOLS_ROOT=D:\BIN"
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
cmake --build build\Release-Python --target unit_tests -j 10
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
cmake --preset Release-Python
cmake --build --preset Release-Python
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
cmake --preset Release-Python
cmake --build --preset Release-Python
```

The Clang template uses `clang` and `clang++`, and chooses `x64-linux`, `arm64-linux`, `x64-osx`, or `arm64-osx` based
on `uname`. The real `.env.sh` is ignored by Git because it contains machine-local paths.

The Bash build, install, and test scripts source `scripts/env.sh` automatically when `.env.sh` exists. Pass
`-SkipEnv` if you intentionally want to use the current shell environment as-is.

### Build Steps

```powershell
# Configure the project using presets (uses $env:VCPKG_ROOT automatically)
cmake --preset Release-Python

# Build
cmake --build --preset Release-Python
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
