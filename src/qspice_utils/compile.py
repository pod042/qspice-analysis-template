# This module contains functions that try to compile C++ code on the same folder of a qspice schematic

import subprocess
import os
from pathlib import Path
from qspice import QschEditor, SimRunner


def gen_DLL_from_cpp(sim: QschEditor, runner: SimRunner, cpp_file: str):
    """
    This function generates the dll file with the c++ files in the same folder that sim is contained.
    Args:   sim = Qscheditor object pointing to schematic with dll object
            runner = SimRunner object configured with a valid qspice executable
            cpp_file = path to the main .cpp or .c file to be compiled
    """
    QSPICE_PATHS = runner.simulator.spice_exe

    # For each of the paths, it will try to compile the c++ files in the folder of sim

    # Checking if cpp_file exists
    if not Path(cpp_file).is_file():
        raise FileExistsError(f"File [{cpp_file}] does not exist!")

    # Checking if cpp_file has cpp suffix
    if Path(cpp_file).suffix.lower() not in [".cpp", ".c"]:
        # Raise error
        raise ValueError("File to be compiled doesn't end in .cpp or .c !")

    # Gets output folder below
    out_folder = runner.output_folder
    if out_folder is None:
        out_folder = Path("./")

    out_file = out_folder / (Path(cpp_file).stem + ".dll")

    # For each path in QSPICE_PATHS, we'll try to search for the Digital Mars (dm) compiler and start a subprocess to compile the files ending in .cpp and .h in sim_folder

    # The command that needs to be evoked is dmc -mn -WD file_name.cpp kernel32.lib
    for path in QSPICE_PATHS:
        # Try to access dmc

        # Define path
        dmc_path = Path(path).parent / "dm" / "bin" / "dmc.exe"

        # Printing attempt
        print(f"Attempting to to compile [{cpp_file}] with dmc in [{dmc_path}]")

        # Try subprocess
        result = subprocess.run(
            [
                os.path.abspath(dmc_path),
                "-mn",
                "-WD",
                os.path.abspath(cpp_file),
                "kernel32.lib",
                "-o",
                os.path.abspath(out_file),
            ],
            cwd=os.path.abspath(out_folder),
            stdout=subprocess.DEVNULL,
        )

        if result.returncode == 0:
            print("Compilation successful, deleting aux files.")
            out_aux_files = []
            out_aux_files.append(Path(os.path.abspath(out_file)).stem + ".obj")
            out_aux_files.append(Path(os.path.abspath(out_file)).stem + ".OBJ")
            out_aux_files.append(Path(os.path.abspath(out_file)).stem + ".def")
            out_aux_files.append(Path(os.path.abspath(out_file)).stem + ".DEF")
            out_aux_files.append(Path(os.path.abspath(out_file)).stem + ".map")
            out_aux_files.append(Path(os.path.abspath(out_file)).stem + ".MAP")

            for auxFile in out_aux_files:
                auxFile_path = os.path.abspath(out_folder / Path(auxFile))
                if Path(auxFile_path).is_file():
                    os.remove(auxFile_path)

            return

        else:
            print("Compilation failed:", result.stderr)

    raise FileNotFoundError(
        f"All attempted compilations failed. Try seeing if the executable path for qspice is set in simrunner."
    )
