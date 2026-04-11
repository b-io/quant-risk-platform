### Python Interface Directory

This directory contains the Python-based API, examples, and notebooks for the Quant Risk Platform.

#### Directory Structure

- `examples/`: Python scripts demonstrating how to use the platform for common risk management tasks.
- `notebooks/`: Jupyter Notebooks providing interactive walkthroughs of the platform's features, including valuation, risk analysis, and VaR.

#### Key Components

The platform's core logic is implemented in C++ and exposed to Python using Pybind11. Python-side dependencies are managed via `requirements.txt` and `pyproject.toml` in the project root.

#### Getting Started

1. Install the Python dependencies and the `qrp` module:
   ```powershell
   .\scripts\install.ps1 -Preset Release-Python
   ```
2. Run an example script:
   ```powershell
   python python\examples\demo_platform.py
   ```
3. Or launch a notebook to explore the API interactively.
