# This module contains helper functions to extract parameter key,values pairs from a .qsch schematic and store them in a dictionary. It automatically converts numerical

import re
from qspice import SpiceEditor, QschEditor
from typing import Any


def eng_string_to_float(eng_string: str) -> float | str:
    """
    Converts an engineering notation string (e.g., '10k', '2.5m', '5Meg') to a float.

    Args:
        eng_string (str): The input string with an optional engineering prefix.

    Returns:
        float: The converted floating-point number.
    """
    # Define the multipliers, including a custom entry for 'Meg'
    multipliers = {
        "f": 1e-15,
        "p": 1e-12,
        "m": 1e-3,
        "": 1e0,  # For strings without a prefix
        "k": 1e3,
        "meg": 1e6,  # Custom 'Meg'
        "g": 1e12,
        "t": 1e15,
    }

    # Use a regular expression to separate the number and the unit/prefix
    # The pattern captures an optional sign, digits/decimals, and an optional suffix
    match = re.match(
        r"([+-]?[0-9]*\.?[0-9]+(?:[eE][+-]?[0-9]+)?)\s*([a-zA-Z]*)",
        eng_string.strip().lower(),
    )

    if match:
        number_part = match.group(1)
        prefix = match.group(2)

        try:
            # Convert the number part to a float
            value = float(number_part)
            # Look up the multiplier (case-insensitive for 'Meg' specifically)
            if prefix in multipliers:
                return value * multipliers[prefix]
            elif prefix.lower() in multipliers:
                return value * multipliers[prefix.lower()]
            else:
                # If the prefix is unknown, return the value as is (or raise an error)
                print(f"Warning: Unknown prefix '{prefix}'. Returning raw value.")
                return value
        except ValueError:
            print(f"Error: Could not parse number part '{number_part}'.")
            return float("nan")  # Return Not a Number on failure
    else:
        # If no match is found, try converting the whole string directly
        try:
            return float(eng_string.strip())
        except ValueError:
            # Just return the original string
            return eng_string


def get_net_params(net: SpiceEditor) -> dict[str, str | float]:
    """
    Returns a dictionary with all the parameters, i.e. '.param X = SMTH' statements from a SpiceEditor object.
    """

    # First gets all the parameter names (all upper case)
    paramNames = net.get_all_parameter_names()

    # Then get all parameter values
    paramValues_eng = [net.get_parameter(name) for name in paramNames]

    # Then convert all numbers from eng format to base values
    paramValues = [eng_string_to_float(eng_str) for eng_str in paramValues_eng]

    # Finally mounts dictionary
    paramDict = dict(zip(paramNames, paramValues))
    return paramDict


def get_sim_params(sim: QschEditor) -> dict[str, Any]:
    """
    Returns a dictionary with all the parameters, i.e. '.param X = SMTH' statements from a SpiceEditor object.
    """

    # First gets all the parameter names (all upper case)
    paramNames = sim.get_all_parameter_names()

    # Then get all parameter values
    paramValues_eng = [sim.get_parameter(name) for name in paramNames]

    # Then convert all numbers from eng format to base values
    paramValues = [eng_string_to_float(eng_str) for eng_str in paramValues_eng]

    # Finally mounts dictionary
    paramDict = dict(zip(paramNames, paramValues))
    return paramDict
