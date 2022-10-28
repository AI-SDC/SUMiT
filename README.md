# SUMiT: Statistical Uncertainty Management Toolkit

The UWE Statistical Uncertainty Management Toolkit (SUMiT) is a software
framework for co-ordinating the application of a range of techniques to large
datasets in the production and publication of public statistics.

*******************************************************************************

## Contents

* `build`: empty folder to be used for building executables
* `data`: contains example data files for testing
* `lib`: contains third-party libraries
* `python`: contains example Python scripts
* `sumit`: contains SUMiT source code

*******************************************************************************

## Building

### Requirements

* C++11 compliant compiler.
* [Java JDK](https://openjdk.org)
* [CMake](https://www.cmake.org "CMake") (>= 3.12)
* [Python](https://www.python.org "Python") (>= 3.6)

### Compiler Options

* `NATIVE_OPT=ON` : Optimise for the native system architecture (CMake default = ON)

### Ubuntu
```
$ sudo apt install python3 cmake
$ git clone --recurse-submodules https://github.com/AI-SDC/SUMiT.git
$ cd SUMiT/build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

### macOS

Notes: if [`brew`](https://brew.sh) is not an option, install [CMake](https://cmake.org/install/).

```
$ brew install cmake python
$ git clone --recurse-submodules https://github.com/AI-SDC/SUMiT.git
$ cd SUMiT/build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

*******************************************************************************

## Running

### Stand-alone

1. Start Server

```
$ java -jar sumit/sumit_server/cell_suppression_server.jar
```

2. Run Suppression Tool

```
$ ./sumit/cell_suppression_tool/cell_suppression_tool
```

### Python

```
$ python3 example.py
```
