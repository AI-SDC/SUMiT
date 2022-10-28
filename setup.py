#!/usr/bin/python3
#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>
#

"""Python setup script for installing SUMiT."""

import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

from setuptools import Extension, find_packages, setup
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    """Creates a CMake extension module."""

    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    """Builds CMake extension."""

    def build_extension(self, ext):
        self.announce("Configuring", level=3)
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        cmake_args = [
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_CXX_COMPILER=g++",
            "-DNATIVE_OPT=OFF",
        ]
        build_args = [
            "--config",
            "Release",
        ]
        if platform.system() == "Windows":
            cmake_args += ["-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE=" + extdir]
            cmake_args += ["-GMinGW Makefiles"]
        else:
            cmake_args += ["-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + extdir]
            build_args += ["-j4"]
        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args, cwd=self.build_temp
        )
        self.announce("Building", level=3)
        subprocess.check_call(
            ["cmake", "--build", "."] + build_args, cwd=self.build_temp
        )
        self.announce("Copying cell suppression tool", level=3)
        path = "/sumit/cell_suppression_tool/"
        exe = "cell_suppression_tool"
        if platform.system() == "Windows":
            exe += ".exe"
        shutil.copy(self.build_temp + path + exe, self.build_lib + path)
        self.announce("Copying cell suppression server", level=3)
        path = "/sumit/sumit_server/"
        exe = "cell_suppression_server.jar"
        shutil.copy(self.build_temp + path + exe, self.build_lib + path)
        shutil.copy(self.build_temp + path + "manifest.mf", self.build_lib + path)


this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text()

setup(
    name="sumit",
    version="1.0.0",
    license="MIT",
    maintainer="Jim Smith",
    maintainer_email="james.smith@uwe.ac.uk",
    description="SUMiT: Statistical Uncertainty Management Toolkit",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/AI-SDC/SUMiT",
    packages=find_packages(),
    #    package_data={"cell_suppression_tool": ["tool"]},
    ext_modules=[CMakeExtension("sumit/sumit")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    python_requires=">=3.8",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Scientific/Engineering",
        "Topic :: Scientific/Engineering :: Information Analysis",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: Microsoft :: Windows :: Windows 10",
        "Operating System :: Microsoft :: Windows :: Windows 11",
    ],
    keywords=[
        "data-privacy",
        "data-protection",
        "privacy",
        "privacy-tools",
        "statistical-disclosure-control",
    ],
)
