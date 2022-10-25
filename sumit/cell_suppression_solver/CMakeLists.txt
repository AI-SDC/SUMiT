#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(SOLVER_SOURCES Solver.cpp UWESolver.cpp)

set(SOLVER_HEADERS Solver.h)

# ##############################################################################
# target: libsolver - solver functions
# ##############################################################################

add_library(solver STATIC ${SOLVER_SOURCES} ${SOLVER_HEADERS})
