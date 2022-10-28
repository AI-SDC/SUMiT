#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(TOOL_SOURCES UWECellSuppression.cpp)

set(TOOL_HEADERS defaults.h optionparser.h)

# ##############################################################################
# target: cell_suppression_tool - stand-alone binary execution
# ##############################################################################

add_executable(cell_suppression_tool ${TOOL_SOURCES} ${TOOL_HEADERS})
target_link_libraries(cell_suppression_tool PUBLIC sumitlib)

file(COPY __init__.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
