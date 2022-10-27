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
        Command line arguments.
    """
    exe = "cell_suppression_tool"
    path = Path(__file__).parent
    if platform.system() == "Windows":
        exe += ".exe"
    command = [os.path.join(path, exe)] + args
    subprocess.run(command, check=True)
