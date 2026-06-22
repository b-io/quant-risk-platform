### Scripts Directory

This directory contains automation scripts for building, installing, initializing, and running the Quant Risk Platform.
Most scripts are provided in both PowerShell (`.ps1`) and Bash (`.sh`) versions for cross-platform compatibility.

#### Core Scripts

| Script      | Purpose                                                           |
|-------------|-------------------------------------------------------------------|
| `build`     | Compiles the project using CMake presets.                         |
| `env`       | Imports the project-local `.env.cmd` or `.env.sh`.                |
| `install`   | Installs Python and C++ dependencies (via vcpkg).                 |
| `init`      | Initializes the SQLite database and loads sample data.            |
| `test`      | Builds and runs unit and integration tests.                       |
| `compute`   | Orchestrates a full valuation, risk, and VaR computation flow.    |
| `visualize` | Provides a quick way to query and display data from the database. |

#### Usage Examples

##### 1. Building the Project

Use a CMake preset (e.g., `Release-Python`) to build everything:

```powershell
# PowerShell (defaults to Release-Python)
.\scripts\build.ps1
```

```bash
# Bash (defaults to Release-Python)
./scripts/build.sh
```

On Windows, `build.ps1`, `install.ps1`, and `test.ps1` automatically import `.env.cmd` when it exists. For manual
`cmake --build` commands from a normal PowerShell terminal, dot-source the same environment first:

```powershell
. .\scripts\env.ps1
cmake --build build\Release-Python --target unit_tests -j 10
```

This is required for MSVC/Ninja builds because `cl.exe` needs the Visual Studio `INCLUDE` and `LIB` environment
variables to find standard headers and libraries.

On Linux/macOS, create `.env.sh` from either `gcc.env.sh.example` or `clang.env.sh.example`. Then use the Bash loader:

```bash
. ./scripts/env.sh
cmake --build build/Release-Python --target unit_tests -j 10
```

`build.sh`, `install.sh`, and `test.sh` source `scripts/env.sh` automatically when `.env.sh` exists. Set
`QRP_SKIP_ENV=1` to disable automatic environment loading.

##### 2. Initializing Data

Set up the database and import sample market data, portfolios, and scenarios:

```powershell
# PowerShell (using default build directory)
.\scripts\init.ps1 -Force
```

##### 3. Running a Full Analysis Flow

Run valuation, sensitivities, and Value-at-Risk for the demo portfolio:

```powershell
# PowerShell (using default build directory)
.\scripts\compute.ps1
```

##### 4. Visualizing Results

List all portfolios or view results of a specific run:

```powershell
# PowerShell
.\scripts\visualize.ps1 portfolios
.\scripts\visualize.ps1 results -Id RUN_VAL_...
```

##### 5. Running Tests

```powershell
# PowerShell
.\scripts\test.ps1
```

#### Note on Parameters

Most scripts support `-BuildDir` and `-Config` (or `-Preset`) to allow working with different build environments. By
default, they now target `build\Release-Python` and the `Release-Python` preset.
