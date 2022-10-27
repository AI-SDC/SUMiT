#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

cmake_minimum_required(VERSION 3.12)

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

# suppression solver - executable
# add_subdirectory(sumit/cell_suppression_solver)
# target_include_directories(solver PUBLIC sumit/sumit_lib)

# suppression server - jar
find_package(Java REQUIRED)
include(UseJava)
add_subdirectory(sumit/sumit_server)

# copy python
file(COPY sumit/__init__.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sumit)
file(GLOB PYTHON_EXAMPLES "python/*")
file(COPY ${PYTHON_EXAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# copy data examples
file(GLOB DATA_EXAMPLES "data/*")
file(COPY ${DATA_EXAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
