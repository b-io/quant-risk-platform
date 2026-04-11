### C++ Core Directory

This directory contains the primary C++ source code and headers for the Quant Risk Platform.

#### Directory Structure

- `include/qrp/`: Public header files, categorized by component:
  - `analytics/`: Core risk and valuation engines.
  - `instruments/`: Financial instrument factories and wrappers.
  - `market/`: Market data snapshots and curve builders.
  - `persistence/`: Database storage and DTO abstractions.
  - `util/`: Shared utility functions (logging, configuration, etc.).
- `src/`: Implementation of the components above.
- `bindings/`: Pybind11-based bindings for exposing C++ functionality to Python.
- `cli/`: Source code for the `qrp_cli` command-line utility.

#### Key Components

- **qrp_core**: The main library containing all risk and valuation logic.
- **qrp_cli**: A standalone executable used for database management, data import, and running valuation/risk flows.
- **qrp_python**: Python module built from the C++ core via pybind11.
