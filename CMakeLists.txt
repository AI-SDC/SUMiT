# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>
# Copyright (C) 2022 Jim Smith <james.smith@uwe.ac.uk>

cmake_minimum_required(VERSION 3.12)

project(SUMIT CXX)
set(PROJECT_DESCRIPTION "SUMIT")
set(PROJECT_VERSION "1.0.0")



# download and install coin-or libraries for solver
set (COINBREW ${PROJECT_SOURCE_DIR}/lib/coinbrew)
###call coinbrew
if(MSVC)
  message(FATAL_ERROR "use of coinbrew to install coin-or libaries not suppprted under windows")
else()
    set(WD ${PROJECT_SOURCE_DIR}/lib)
    if( EXISTS ${COINBREW} AND NOT MSVC )
        #fetch clp
        execute_process(COMMAND ${COINBREW} fetch Clp --latest-release --no-third-party
                        #WORKING_DIRECTORY ${WD}
                        RESULT_VARIABLE COINBREW_FETCH_CLP)
        if(NOT COINBREW_FETCH_CLP EQUAL "0")
            message(FATAL_ERROR "coinbrew failed to fetch Clp" working in ${WD})
        endif()
        #fetch osi
            execute_process(COMMAND ${COINBREW} fetch Osi --latest-release --no-third-party
                        #WORKING_DIRECTORY ${WD}
                        RESULT_VARIABLE COINBREW_FETCH_OSI)
        if(NOT COINBREW_FETCH_OSI EQUAL "0")
            message(FATAL_ERROR "coinbrew failed to fetch Osi")
        endif()
        #build osi
            execute_process(COMMAND ${COINBREW} build Osi --verbosity 2 
                        --no-third-party
                        --parallel-jobs 4 --build-dir build -p dist
                        --static --tests all --build-dir build
                        #WORKING_DIRECTORY ${WD}
                        RESULT_VARIABLE COINBREW_BUILD_OSI)
        if(NOT COINBREW_BUILD_OSI EQUAL "0")
            message(FATAL_ERROR "coinbrew failed to build Osi Clp")
        endif()
        #build clp
            execute_process(COMMAND ${COINBREW} build Clp --verbosity 2 
                    --no-third-party
                    --parallel-jobs 4 --build-dir build -p dist
                    --static --tests all --build-dir build
                        #WORKING_DIRECTORY ${WD}
                        RESULT_VARIABLE COINBREW_BUILD_CLP)
        if(NOT COINBREW_BUILD_CLP EQUAL "0")
            message(FATAL_ERROR "coinbrew failed to build Clp")
        endif()
    else() 
      message(FATAL_ERROR "coinbrew not found:\n" ${COINBREW})
    endif()
endif()



add_subdirectory(lib)











#main compilation flags etc.
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
add_subdirectory(lib)

# suppression solver - executable
#add_subdirectory(sumit/cell_suppression_solver)
# target_include_directories(solver PUBLIC sumit/sumit_lib lib/include)

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
