# SUMiT: Statistical Uncertainty Management Toolkit

The UWE Statistical Uncertainty Management Toolkit (SUMiT) is a software
framework for co-ordinating the application of a range of techniques to large
datasets in the production and publication of public statistics.

*******************************************************************************

## Contents

* `build`: empty folder to be used for building executables
* `data`: contains example data files for testing
* `doc`: contains files for generating Doxygen documentation
* `lib`: contains third-party libraries for unit testing, pybind, etc.
* `python`: contains example Python scripts
* `sumit`: contains SUMiT source code (currently only the Unpicker class)

*******************************************************************************

## Building

### Requirements

Stand-alone binary:

* C++11 compliant compiler.
* [CMake](https://www.cmake.org "CMake") (>= 3.12)

Python library:

* All of the above for building the stand-alone executable.
* [Python](https://www.python.org "Python") (>= 3.6)

### Compiler Options

* `MAIN=ON` : Build the stand-alone main executable (CMake default = ON)
* `PYLIB=ON` : Build the Python library (CMake default = OFF)
* `NATIVE_OPT=ON` : Optimise for the native system architecture (CMake default = ON)
* `SANITIZE=ON` : Enable [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer) and [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) - compile as Debug (CMake default = OFF)

### Ubuntu
```
$ sudo apt install python3 python3-dev cmake
$ git clone --recurse-submodules https://github.com/AI-SDC/SUMiT.git
$ cd sumit/build
$ cmake -DCMAKE_BUILD_TYPE=Release -DPYLIB=ON ..
$ make
```

### macOS

```
$ brew install cmake python
$ git clone --recurse-submodules https://github.com/AI-SDC/SUMiT.git
$ cd sumit/build
$ cmake -DCMAKE_BUILD_TYPE=Release -DPYLIB=ON ..
$ make
```

### Documentation

[Doxygen](http://www.doxygen.nl/download.html) + [graphviz](https://www.graphviz.org/download/)

After running CMake:

```
$ make doc
```

*******************************************************************************

## Running

To run the unpicker:

### Stand-alone

Arguments: (1) name of an input file; (2) name of results file.

```
$ ./sumit/main test.jj result.txt
```

### Python

```
$ python3 unpicker.py
```
