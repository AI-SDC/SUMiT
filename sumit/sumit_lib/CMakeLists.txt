#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(LIB_SOURCES
    CSVWriter.cpp
    CellStore.cpp
    Eliminate.cpp
    Evaluation.cpp
    EvaluationCache.cpp
    GAProtection.cpp
    GroupedGAProtection.cpp
    Groups.cpp
    IncrementalGAProtection.cpp
    JJData.cpp
    LegacyTabularPartitioning.cpp
    Logger.cpp
    NoPartitioning.cpp
    PartitionData.cpp
    Partitioning.cpp
    ProgressLog.cpp
    SamplesLog.cpp
    ServerConnection.cpp
    Solver.cpp
    System.cpp
    TabularData.cpp
    Unpicker.cpp
    stdafx.cpp)

set(LIB_HEADERS
    CSVWriter.h
    CellStore.h
    Eliminate.h
    Evaluation.h
    EvaluationCache.h
    GAProtection.h
    GroupedGAProtection.h
    Groups.h
    IncrementalGAProtection.h
    Individual.h
    JJData.h
    LegacyTabularPartitioning.h
    Logger.h
    MersenneTwister.h
    NoPartitioning.h
    PartitionData.h
    Partitioning.h
    ProgressLog.h
    SamplesLog.h
    ServerConnection.h
    Solver.h
    System.h
    TabularData.h
    UWECellSuppression.h
    Unpicker.h
    stdafx.h
    targetver.h)

# ##############################################################################
# target: libsumitlib - main functions
# ##############################################################################

add_library(sumitlib STATIC ${LIB_SOURCES} ${LIB_HEADERS})
