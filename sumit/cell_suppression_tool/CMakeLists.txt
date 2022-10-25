#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(TOOL_SOURCES UWECellSuppression.cpp)

set(TOOL_HEADERS defaults.h optionparser.h)

# ##############################################################################
# target: tool - stand-alone binary execution
# ##############################################################################

add_executable(tool ${TOOL_SOURCES} ${TOOL_HEADERS})
target_link_libraries(tool PUBLIC sumitlib)
