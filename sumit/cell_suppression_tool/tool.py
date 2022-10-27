#!/usr/bin/python3
#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>
#

"""Python script for executing SUMiT."""

import os
import platform
import subprocess
from pathlib import Path


def run() -> None:
    exe = "cell_suppression_tool"
    path = Path(__file__).parent
    if platform.system() == "Windows":
        exe += ".exe"
    subprocess.run([os.path.join(path, exe)])
