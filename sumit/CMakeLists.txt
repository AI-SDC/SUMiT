#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(UNPICKER_SOURCES
    JJData.cpp
    Unpicker.cpp)

set(UNPICKER_HEADERS
    JJData.h
    Unpicker.h
    UWECellSuppression.h)

# ##############################################################################
# target: libunpicker - main functions
# ##############################################################################

add_library(unpicker STATIC ${UNPICKER_SOURCES} ${UNPICKER_HEADERS})
target_link_libraries(unpicker PUBLIC m)

# ##############################################################################
# target: main - stand-alone binary execution
# ##############################################################################

if(MAIN)
  add_executable(main main.cpp)
  target_link_libraries(main PUBLIC unpicker)
endif()

# ##############################################################################
# target: sumit.so / .pyd - Python library
# ##############################################################################

if(PYLIB)
  set(PY_SOURCES pybind_wrapper.cpp)
  pybind11_add_module(sumit ${PY_SOURCES})
  target_link_libraries(sumit PUBLIC unpicker)
endif()
