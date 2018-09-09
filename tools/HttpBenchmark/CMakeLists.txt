cmake_minimum_required(VERSION 3.2)
project(HttpBenchmark CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system program_options)

set(SOURCE_FILES
        main.cpp)

include_directories(
        ../
        ../../src/LibFastJsonWriter
        ../../src/LibForumContext
        ../../src/LibForumHelpers
        ../../src/LibHttp
        ${Boost_INCLUDE_DIRS})

add_executable(HttpBenchmark ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(HttpBenchmark
        ForumContext
        Http
        atomic
        ${Boost_LIBRARIES})
