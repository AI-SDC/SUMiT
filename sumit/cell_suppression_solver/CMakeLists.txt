#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(SOLVER_SOURCES Solver.cpp UWESolver.cpp)

set(SOLVER_HEADERS Solver.h)

# ##############################################################################
# target: cell_suppression_solver - stand-alone binary execution
# ##############################################################################

set(COIN_LIB "${CMAKE_SOURCE_DIR}/build/dist/lib")

find_library(CoinUtils NAMES CoinUtils PATHS ${COIN_LIB} REQUIRED)
find_library(Osi NAMES Osi PATHS ${COIN_LIB} REQUIRED)
find_library(Clp NAMES Clp PATHS ${COIN_LIB} REQUIRED)
find_library(OsiClp NAMES OsiClp PATHS ${COIN_LIB} REQUIRED)

add_executable(cell_suppression_solver ${SOLVER_SOURCES} ${SOLVER_HEADERS})
target_link_libraries(cell_suppression_solver
    PUBLIC sumitlib ${CoinUtils} ${Osi} ${Clp} ${OsiClp})
