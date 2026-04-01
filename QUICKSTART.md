# Quickstart Guide

This guide explains supported build workflows on Windows using CMake and how to enable optional Python bindings safely.

## Supported configurations
- C++ only: Debug, Release, RelWithDebInfo
- Python bindings on Windows: Release, RelWithDebInfo (Debug requires a matching debug CPython and is not supported by default)

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
# Build and run all tests
.\scripts\build_and_test.ps1

# Build and install the Python bindings
.\scripts\build_and_install.ps1
```

### On Linux / WSL (Bash):
```bash
# Build and run all tests
chmod +x scripts/*.sh
./scripts/build_and_test.sh

# Build and install the Python bindings
./scripts/build_and_install.sh
```

## 4. Run a Demo

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
./build/Debug/qrp_cli price --market data/market/base_market.json --portfolio data/portfolios/demo_macro_book.json
```

## 3. Artifacts and Outputs

- **Database**: Persistent data is stored in `var/quant_risk_platform.sqlite` by default.
- **Logs**: Application logs are written to `var/logs/`.
- **Binaries**: Built executables (`qrp_cli`, `unit_tests`, `integration_tests`) are located in the build directory under `cpp/cli/` and `tests/`.

---

## Advanced: Manual Build (CMake)

If you need to customize the build, you can use standard CMake commands.

### Prerequisites

The platform depends on:
- **C++ Compiler**: Supporting C++20 (MSVC 2022, GCC 11+, Clang 13+).
- **vcpkg**: For managing C++ dependencies (QuantLib, fmt, GTest, nlohmann-json).

### Build Steps

```bash
# Configure the project with vcpkg toolchain
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Debug
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
