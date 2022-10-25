#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(PY_SOURCES pybind_wrapper.cpp)

# ##############################################################################
# target: sumit.so / .pyd - Python library
# ##############################################################################

if(PYLIB)
  set(PY_SOURCES pybind_wrapper.cpp)
  pybind11_add_module(sumit ${PY_SOURCES})
  target_link_libraries(sumit PUBLIC sumitlib)
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py"
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(GLOB PYTHON_EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/../../python/*.py")
  file(COPY ${PYTHON_EXAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
