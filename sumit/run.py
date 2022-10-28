#!/usr/bin/python3
#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>
#

"""Python script for executing SUMiT."""

import os
import platform
import subprocess
from pathlib import Path


def run(args: list[str]) -> None:
    """Runs the cell suppression tool.

    Parameters
    ----------
    args : list[str]
        Command line arguments to the cell suppression tool.
    """
    path = Path(__file__).parent

    # run cell suppression server
    exe_server = "cell_suppression_server.jar"
    dir_server = "sumit_server"
    command = ["java", "-jar"]
    command += [os.path.join(path, dir_server, exe_server)]
    subprocess.run(command, check=True)

    # run cell suppression tool
    exe_tool = "cell_suppression_tool"
    dir_tool = "cell_suppression_tool"
    if platform.system() == "Windows":
        exe_tool += ".exe"
    command = [os.path.join(path, dir_tool, exe_tool)] + args
    subprocess.run(command, check=True)
