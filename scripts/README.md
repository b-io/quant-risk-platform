### Scripts Directory

This directory contains automation scripts for building, installing, initializing, and running the Quant Risk Platform.
Most scripts are provided in both PowerShell (`.ps1`) and Bash (`.sh`) versions for cross-platform parity.

#### Core Scripts

| Script      | Purpose                                                           |
|-------------|-------------------------------------------------------------------|
| `build`     | Compiles the project using CMake presets.                         |
| `compute`   | Orchestrates a full valuation, risk, and VaR computation flow.    |
| `coverage/` | Contains language-specific coverage metric helpers.               |
| `env`       | Imports the project-local `.env.cmd` or `.env.sh`.                |
| `init`      | Initializes the SQLite database and loads sample data.            |
| `inspect`   | Provides a quick way to inspect persisted database tables and run outputs. |
| `install`   | Installs Python and C++ dependencies (via vcpkg).                 |
| `test`      | Builds and runs unit and integration tests.                       |

#### Usage Examples

##### 1. Building the Project

Use a CMake preset (for example `dev`) to build everything:

```powershell
# PowerShell (defaults to dev)
.\scripts\build.ps1
```

```bash
# Bash (defaults to dev)
./scripts/build.sh
```

On Windows, the PowerShell wrapper scripts automatically import `.env.cmd` when it exists. For manual `cmake --build`
commands from a normal PowerShell terminal, dot-source the same environment first:

```powershell
. .\scripts\env.ps1
cmake --build build\dev --target unit_tests -j 10
```

This is required for MSVC/Ninja builds because `cl.exe` needs the Visual Studio `INCLUDE` and `LIB` environment
variables to find standard headers and libraries.

On Linux/macOS, create `.env.sh` from either `gcc.env.sh.example` or `clang.env.sh.example`. Then use the Bash loader:

```bash
. ./scripts/env.sh
cmake --build build/dev --target unit_tests -j 10
```

The Bash wrapper scripts source `scripts/env.sh` automatically when `.env.sh` exists. Pass `-SkipEnv` to disable
automatic environment loading.

##### 2. Initializing Data

Set up the database and import sample market data, portfolios, and scenarios:

```powershell
# PowerShell (using default build directory)
.\scripts\init.ps1 -Force
```

```bash
# Bash (using default build directory)
./scripts/init.sh -Force
```

##### 3. Running a Full Analysis Flow

Run valuation, sensitivities, and Value-at-Risk for the demo portfolio:

```powershell
# PowerShell (using default build directory)
.\scripts\compute.ps1
```

```bash
# Bash (using default build directory)
./scripts/compute.sh
```

##### 4. Inspecting Results

List all portfolios or view results of a specific run:

```powershell
# PowerShell
.\scripts\inspect.ps1 portfolios
.\scripts\inspect.ps1 results -Id RUN_VAL_...
.\scripts\inspect.ps1 risk -Id RUN_RISK_...
.\scripts\inspect.ps1 summary -DbFile var\quant_risk_platform.sqlite
```

```bash
# Bash
./scripts/inspect.sh portfolios
./scripts/inspect.sh results -Id RUN_VAL_...
./scripts/inspect.sh risk -Id RUN_RISK_...
./scripts/inspect.sh summary -DbFile var/quant_risk_platform.sqlite
```

##### 5. Running Tests

```powershell
# PowerShell
.\scripts\test.ps1
```

```bash
# Bash
./scripts/test.sh
```

#### Note on Parameters

Most scripts support `-BuildDir` and `-Config` (or `-Preset`) to allow working with different build environments. By
default, they now target `build\dev` and the `dev` preset.

The public option names are intentionally shared across PowerShell and Bash where applicable:

- `-BuildDir`: select an existing build directory for CLI/test/database scripts.
- `-Config`: select an existing build configuration for CLI/test/database scripts.
- `-Coverage`: request C++ coverage generation in test scripts.
- `-CppCoverageMinLine`: set the C++ coverage threshold used by test scripts.
- `-DbFile`: select the SQLite database for inspection.
- `-ExtraArgs`: pass remaining arguments to CMake configure in build scripts.
- `-Force`: reinitialize existing local data during database initialization.
- `-Id`: select the run identifier for inspection.
- `-LogLevel`: select the compute-flow log level.
- `-MarketFile`: select the market-data input file for database initialization.
- `-PortfolioFile`: select the portfolio input file for database initialization.
- `-PortfolioId`: select the compute-flow portfolio.
- `-Preset`: select a CMake preset for build/install scripts.
- `-PythonCoverageMinLine`: set the Python coverage threshold used by test scripts.
- `-PythonExecutable`: select the Python executable used by test and coverage scripts.
- `-ScenarioFile`: select the scenario input file for database initialization.
- `-ScenarioSetId`: select the compute-flow scenario set.
- `-SkipEnv`: skip automatic project environment loading.
- `-SkipCppCoverage`: skip C++ coverage generation in test scripts.
- `-SkipPythonCoverage`: skip Python coverage generation in test scripts.
- `-SnapshotId`: select the compute-flow market snapshot.
- `-Table`: select the database table or result category for inspection.

MSVC C++ coverage honors `QRP_MSVC_COVERAGE_GTEST_FILTER` when present. The
default excludes the application workflow smoke fixture from coverage
instrumentation because it is run normally before coverage but currently
terminates inside the MSVC coverage collector.
