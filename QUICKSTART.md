# Quickstart Guide

Follow these steps to build the C++ core and run the Python demo using the provided automation scripts.

## 1. Quick Build and Test (Recommended)

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

## 2. Run a Demo

After building, you can run a Python demo or use the C++ CLI.

### Python Demo
```bash
# Ensure you are in a virtual environment with requirements installed
python -m venv venv
source venv/bin/activate  # On Windows: .\venv\Scripts\activate
pip install -r requirements.txt

# Run the demo
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
