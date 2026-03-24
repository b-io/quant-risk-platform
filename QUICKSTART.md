# Quickstart Guide

Follow these steps to build the C++ core and run the Python demo.

## Prerequisites

The platform depends on several C++ libraries and Python tools.

### C++ Dependencies
- **fmt**: Formatting library.
- **GoogleTest**: Testing framework.
- **nlohmann-json**: JSON for Modern C++.
- **pybind11**: C++11 bindings for Python.
- **QuantLib**: Financial instrument and pricing library.

### Tools
- **C++ Compiler**: GCC 11+, Clang 13+, or MSVC 2022+ (supporting C++20).
- **CMake**: 3.21 or higher.
- **Python**: 3.9 or higher.

## Installation via Package Manager

We use **vcpkg** (C++) and **pip** (Python) to manage dependencies automatically.

### WSL (Windows Subsystem for Linux) Setup
If you are on Windows and prefer a Linux-first environment (highly recommended for this project), follow these steps to set up WSL:

1.  **Install WSL**:
    Open PowerShell as Administrator and run:
    ```powershell
    # Install WSL and the Ubuntu distribution explicitly
    wsl --install -d Ubuntu
    
    # Set Ubuntu as the default distribution
    wsl --set-default Ubuntu
    ```
    This installs WSL, the Ubuntu distribution, and sets it as the default. Restart your computer if prompted.

2.  **Install Mandatory System Dependencies**:
    Once inside your WSL distribution (e.g., Ubuntu), you **must** install the following packages for `vcpkg` and the build tools to work:
    ```bash
    sudo apt-get update
    sudo apt-get install -y autoconf autoconf-archive automake build-essential cmake curl git libtool ninja-build pkg-config tar unzip zip
    ```

3.  **Verify the Installation**:
    Run `wsl --list --verbose` to check if a distribution is installed and running.

4.  **Using WSL in this Project**:
    -   Open your terminal (e.g., Windows Terminal or VS Code Terminal) and select **Ubuntu** (or your chosen WSL distro).
    -   Navigate to your project directory (e.g., `cd /mnt/d/IT/DEV/C++/quant-risk-platform`).
    -   Follow the Linux-specific build instructions below.

### 1. Install vcpkg (If not already installed)
If you don't have `vcpkg`, follow these steps in your home or development directory (outside this project):

**On Linux / WSL (Native Home Directory - Recommended):**
```bash
# Navigate to your native Linux home directory
cd ~
mkdir -p dev && cd dev

# Clone and bootstrap vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Add the vcpkg directory to your PATH (add this to ~/.bashrc for permanence)
export PATH=$PATH:$(pwd)
```

**On Windows (PowerShell):**
```powershell
# Navigate to an external directory (e.g., C:\dev)
cd C:\
mkdir dev
cd dev

# Clone and bootstrap vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Add the vcpkg directory to your PATH
$env:PATH += ";$PWD"
```

### 2. Build the C++ Core with vcpkg

**Note: vcpkg path**
In the command below, we assume `vcpkg` is installed at `~/dev/vcpkg` (the path to your vcpkg installation). 

- If you installed it elsewhere, replace `~/dev/vcpkg` with your actual path.
- If you don't know where it is, run: `find ~ -name vcpkg.cmake 2>/dev/null`

```bash
# Configure the project with the vcpkg toolchain
cmake --preset debug -DCMAKE_TOOLCHAIN_FILE=~/dev/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build the core library, CLI, and bindings
cmake --build --preset debug
```

### 3. Set up Python Environment
```bash
python -m venv venv
source venv/bin/activate  # On Windows PowerShell use: .\venv\Scripts\activate
pip install -r requirements.txt
```

---

## Manual Build (No vcpkg)

### 1. Build the C++ Core

We use CMake presets for a consistent build experience.

```bash
# Configure the project
cmake --preset debug

# Build the core library, CLI, and bindings
cmake --build --preset debug
```

### 2. Set up Python Environment

It is recommended to use a virtual environment.

```bash
python -m venv venv
source venv/bin/activate  # On Windows use: venv\Scripts\activate
pip install nlohmann-json # If needed for python-side scripts
```

### 3. Run the Python Demo

The Python demo showcases loading market data, pricing a portfolio, and computing risk. Ensure the built `qrp_core` module is in your `PYTHONPATH`.

```bash
# Add the build directory to PYTHONPATH (adjust path based on your OS/build folder)
export PYTHONPATH=$PYTHONPATH:$(pwd)/build/debug/cpp/bindings

# Run the demo script
python python/examples/demo_platform.py
```

### 4. Run C++ Tests

Verify the installation by running the unit and integration tests.

```bash
ctest --preset debug
```

### 5. Run the CLI

You can also use the C++ CLI for basic pricing tasks.

```bash
./build/debug/cpp/cli/qrp_cli price --market data/market/base_market.json --portfolio data/portfolios/demo_macro_book.json
```
