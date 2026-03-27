# QSPICE Analysis Template

A Python-based jupyter notebook template for automating QSPICE simulations and performing data analysis. This project provides a structured approach to running simulations programmatically, processing results, and generating insights from QSPICE circuit simulations. It's based on Nunobrun's modules 

## Features

- **Programmatic Simulation Control**: Use Python to modify parameters, run simulations, and collect data automatically.
- **Data Analysis Integration**: Built-in tools for processing simulation outputs and creating visualizations.
- **DLL Generation**: Automatic compilation of C/C++ code into DLLs for use in QSPICE simulations.
- **Example Templates**: Includes two comprehensive examples to get started quickly.
- **Modular Utilities**: Custom utilities in `qspice_utils` for common tasks like parameter management and signal sampling.

## Prerequisites

Before using this template, ensure you have the following software installed:

- **Python 3.11 or higher**: The project requires Python 3.11+ for compatibility with the dependencies.
- **QSPICE (Free)**: Download and install QSPICE from the official website (https://www.qorvo.com/design-hub/design-tools/interactive/qspice). This is the core simulation engine used by the template.
- **Poetry (Free)**: A dependency management tool for Python. Install it from https://python-poetry.org/docs/#installation. Poetry handles the virtual environment and package installation.

## Installation

1. **Clone the Repository**:
   ```bash
   git clone <repository-url>
   cd qspice-analysis-template
   ```

2. **Install Dependencies with Poetry**:
   ```bash
   poetry install
   ```

   This will create a virtual environment and install all required Python packages, including `qspice`, `pandas`, and `ipykernel`.

3. **Activate the Virtual Environment** (optional, but recommended):
   ```bash
   poetry shell
   ```

## Usage

1. **Launch Jupyter Notebook**:
   ```bash
   jupyter notebook
   ```
   Or use VS Code's Jupyter extension to open the notebooks.

2. **Run the Examples**:
   - Open `notebooks/qspice_analysis_template.ipynb`.
   - Follow the cells to run the provided examples.
   - Modify parameters and simulations as needed for your own circuits.

3. **Customize for Your Project**:
   - Place your QSPICE schematic files (`.qsch`) in the `sim/` directory.
   - Update the notebooks or create new ones to automate your simulations.
   - Use the utilities in `src/qspice_utils/` for common tasks.

## Examples

### Example 1: Simple Simulation
A basic proof-of-concept simulation demonstrating voltage plotting on two nodes. It covers:
- Loading a QSPICE schematic.
- Modifying parameters.
- Running the simulation.
- Plotting results.

### Example 2: Boost PFC Converter
A more advanced example with a bridgeless totem-pole boost PFC converter. Features:
- Batch simulations with different parameters.
- Signal sampling using callbacks.
- Data collection and visualization for multiple runs.

Both examples include C++ code that gets compiled into DLLs for integration with QSPICE.

## Project Structure

```
qspice-analysis-template/
├── data/                    # Simulation data and netlists
├── notebooks/               # Jupyter notebooks with examples
├── sim/                     # QSPICE schematic files and C++ code
├── src/qspice_utils/        # Utility modules for simulation tasks
├── tests/                   # Test files
├── pyproject.toml           # Poetry configuration and dependencies
└── README.md                # This file
```

## License

This project is licensed under the MIT License. See the LICENSE file for details.
