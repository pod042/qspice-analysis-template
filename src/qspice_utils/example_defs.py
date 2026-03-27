# This module implements some utility functions and classes for the examples

# This function build the verilog_config for example 2
def build_verilog_config_ex2() -> dict[str, list[str]]:
    # Every port is of type double
    # Inputs first

    ports = list()
    ports.append("in,d")
    ports.append("in,d")
    ports.append("in,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")
    ports.append("out,d")

    return {"X1": ports}
