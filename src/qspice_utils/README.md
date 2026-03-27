# Qspice utilities

This module contains some python helper functions for automating stuff when running scripted Qspice simulations. It has four submodules:

## compile.py

It has a function that tries to call the standard C++ compiler shipped with Qspice to auto-generate a .dll file for a Ø device in the schematic (i.e. the object that runs the .dll file).

## example_defs.py

This file simply has a helper function for example 2, which was separated from the main notebook for readability.

## params.py

It contains some functions to return a dictionary with all the parameters from a netlist or schematic file. It may be used with other simulations as well.

## pre_process.py

This file contains some functions for pre-processing the data in the examples. An example callback function is given. A function to iterate on the results of the callback and generate a grouped dataframe of the results is also given.