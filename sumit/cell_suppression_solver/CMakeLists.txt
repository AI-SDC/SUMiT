#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(SOLVER_SOURCES Solver.cpp UWESolver.cpp)

set(SOLVER_HEADERS Solver.h)

# ##############################################################################
# target: cell_suppression_solver - stand-alone binary execution
# ##############################################################################

add_executable(cell_suppression_solver ${SOLVER_SOURCES} ${SOLVER_HEADERS})
target_link_libraries(cell_suppression_solver PUBLIC sumitlib)
