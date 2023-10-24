cmake_minimum_required(VERSION 3.24)
project(WeakMemoryModelMock)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tso_model src/main_TSO.cpp)
target_link_libraries(tso_model PUBLIC program_lib execution_lib storage_lib)

add_executable(pso_model src/main_PSO.cpp)
target_link_libraries(pso_model PUBLIC program_lib execution_lib storage_lib)

add_executable(sc_model src/main_SC.cpp)
target_link_libraries(sc_model PUBLIC program_lib execution_lib storage_lib)


add_library(storage_lib SHARED)
target_include_directories(storage_lib PUBLIC src/Storage)
target_sources(storage_lib PUBLIC
        src/Storage/src/Storage.cpp
        src/Storage/src/TotalStoreOrderStorageManager.cpp
        src/Storage/src/PartialStoreOrderStorageManager.cpp
        src/Storage/src/SequentialConsistencyStorageManager.cpp
        src/Storage/src/StorageLogger.cpp
        )
target_link_libraries(storage_lib PUBLIC program_lib)

add_library(execution_lib SHARED)
target_include_directories(execution_lib PUBLIC src/Execution)
target_sources(execution_lib PUBLIC
        src/Execution/src/ThreadManager.cpp
        src/Execution/src/Thread.cpp
        src/Execution/src/Executor.cpp
        )
target_link_libraries(execution_lib PUBLIC program_lib storage_lib)

add_library(program_lib SHARED)
target_include_directories(program_lib PUBLIC src/Program)
target_sources(program_lib PUBLIC
        src/Program/src/Program.cpp
        src/Program/src/Parser.cpp
        src/Program/src/Instructions.cpp
        )

add_executable(test test/doctest_main.cpp)
target_sources(test PUBLIC
        test/ParserTest.cpp
        test/StorageManagerTest.cpp)
target_link_libraries(test PUBLIC program_lib storage_lib)
