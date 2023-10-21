cmake_minimum_required(VERSION 3.24)
project(WeakMemoryModelMock)

set(CMAKE_CXX_STANDARD 20)

add_executable(weak_memory_model src/main.cpp)
target_link_libraries(weak_memory_model PUBLIC
        parser_lib
        executor_lib
        storage_lib
        )

add_library(storage_lib STATIC)
target_include_directories(storage_lib PUBLIC include)
target_sources(storage_lib PUBLIC
        src/Storage.cpp
        src/StorageManager.cpp
        )

add_library(executor_lib STATIC)
target_include_directories(executor_lib PUBLIC include)
target_sources(executor_lib PUBLIC
        src/ThreadManager.cpp
        src/Thread.cpp
        )

add_library(parser_lib STATIC)
target_include_directories(parser_lib PUBLIC include)
target_sources(parser_lib PUBLIC
        src/Program.cpp
        src/Parser.cpp
        src/Instructions.cpp
        )

add_executable(test test/doctest_main.cpp)
target_sources(test PUBLIC test/ParserTest.cpp)
target_link_libraries(test PUBLIC parser_lib)
