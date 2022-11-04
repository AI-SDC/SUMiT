# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>
# Copyright (C) 2022 Jim Smith <james.smith@uwe.ac.uk>

cmake_minimum_required(VERSION 3.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

project(SUMIT CXX)
set(PROJECT_DESCRIPTION "SUMIT")
set(PROJECT_VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra ")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused ")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-qual")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wredundant-decls")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winit-self")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuninitialized")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmaybe-uninitialized")
  endif()
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(CMAKE_CXX_FLAGS_DEBUG "-g3")

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(NATIVE_OPT "Optimise for the native architecture" ON)
if(NATIVE_OPT)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffat-lto-objects")
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} -fno-semantic-interposition")
endif()

# sumit libs
add_subdirectory(sumit/sumit_lib)

# suppression tool - executable
add_subdirectory(sumit/cell_suppression_tool)
target_include_directories(cell_suppression_tool PUBLIC sumit/sumit_lib)

# coin-or libaries used by solver
message(INFO " Running coinbrew")

if(WIN32)
  message(FATAL_ERROR "coinbrew installation not suppprted on windows")
else()
  file(GLOB COIN_OR "lib/*")
  file(COPY ${COIN_OR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  execute_process(
    COMMAND bash runcoinbrew.sh
    OUTPUT_FILE coin_log.txt
    ERROR_FILE coin_errors.txt
    RESULT_VARIABLE RUNCOIN_RES)
  if(NOT RUNCOIN_RES EQUAL "0")
    message(FATAL_ERROR "coinbrew failed with message " ${RUNCOIN_RES})
  endif()
endif()

# suppression solver - executable
add_subdirectory(sumit/cell_suppression_solver)
target_include_directories(cell_suppression_solver
    PUBLIC sumit/sumit_lib ${CMAKE_BINARY_DIR}/dist/include/coin)

# suppression server - jar
find_package(Java REQUIRED)
include(UseJava)
add_subdirectory(sumit/sumit_server)

# copy python
file(COPY sumit/__init__.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sumit)
file(COPY sumit/run.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sumit)
# copy examples
file(GLOB PYTHON_EXAMPLES "python/*")
file(COPY ${PYTHON_EXAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# copy data examples
file(GLOB DATA_EXAMPLES "data/*")
file(COPY ${DATA_EXAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
