# This module contains helper functions to pre-process the sim data of qspice
import pandas as pd
import numpy as np
import os
import re
from qspice import RawRead, SimRunner
from pathlib import Path


def sample_voltages_callback(
    raw_file: Path | None, log_file: Path | None, simParams: dict
):
    """
    Samples any voltage signal whose node's name ends in '_sample' in the schematic based on a signal called 'sampleflag'. Also samples time. Returns a dataframe of the format:
    |sim_id|time|trace_1|trace_2|...|trace_n|simParams_dict|
    """
    if raw_file is None or log_file is None:
        raise ValueError("Raw or log file is None; cannot process simulation data.")

    print("Handling the simulation data of %s" % raw_file)

    # Gets sim name
    simId = raw_file.name

    # Reads raw file
    rawData = RawRead(raw_file)

    # First we get every trace names
    traceNames = rawData.get_trace_names()

    # Then we try to find a voltage signal called V(sampleFlag). If we can't we raise an error
    if "V(sampleflag)" not in traceNames:
        raise ValueError(f"Required signal V(sampleFlag) is not in schematic!")

    # Then search for strs of the pattern 'V(signalname_sample)'
    sample_pattern = r"V\(([^)]+_sample)\)"

    # Extracting only the names and adding time
    sigs_to_sample = ["time"] + [
        s for s in traceNames if re.fullmatch(sample_pattern, s)
    ]

    # Extracting the signal names
    sigNames = ["time"]
    for s in traceNames:
        m = re.fullmatch(sample_pattern, s)
        if m:
            sigNames.append(m.group(1))

    # Sampling the sigs
    sampled_sigs = {}
    sFlag = rawData.get_wave("V(sampleflag)")
    ix_sample = np.nonzero(sFlag)
    for i in range(len(sigs_to_sample)):
        # Sample sig
        sig = sigs_to_sample[i]
        sigName = sigNames[i]
        sampled_sig = rawData.get_wave(sig)[ix_sample]
        # Add to dict
        sampled_sigs.update({sigName: sampled_sig})

    # Mounts table to output
    idcolumns = [simId] * len(sampled_sigs["time"])
    dataDict = {"simId": idcolumns}
    dataDict.update(sampled_sigs)
    df = pd.DataFrame(dataDict)

    # Adds simParams as a last column
    df["simParams"] = pd.Series([simParams] * len(df), index=df.index, dtype="object")

    # Also manually deletes qraw file
    os.remove(raw_file)

    # Manually deletes the log file
    os.remove(log_file)

    # Manually deletes the net file
    os.remove(os.path.splitext(raw_file)[0] + ".net")

    # Final print
    print("Callback of raw file %s finished" % raw_file)

    return df


def createTable(runner: SimRunner):
    """
    Given a simRunner object sim with specific series data, return all the
    saved datat in a dataframe
    """
    dataTable = pd.DataFrame([])

    for data in runner:
        dataTable = pd.concat([dataTable, data], ignore_index=True)

    return dataTable
