cmake_minimum_required(VERSION 3.2)
project(HttpTests CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS unit_test_framework system)
find_package(ICU)

set(SOURCE_FILES
        main.cpp
        ParserTests.cpp
        ResponseBuilderTests.cpp
        TrieTests.cpp)

include_directories(
        ../../src
        ../../src/LibHttp
        ${Boost_INCLUDE_DIRS})

add_executable(HttpTests
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(HttpTests
        Http
        ${Boost_LIBRARIES})

add_test(NAME Http_Tests COMMAND HttpTests)
