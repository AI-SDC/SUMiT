#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(SOLVER_SOURCES Solver.cpp UWESolver.cpp)

set(SOLVER_HEADERS Solver.h)

# ##############################################################################
# target: solver - stand-alone binary execution
# ##############################################################################

add_executable(solver ${SOLVER_SOURCES} ${SOLVER_HEADERS})
target_link_libraries(solver PUBLIC sumitlib)
