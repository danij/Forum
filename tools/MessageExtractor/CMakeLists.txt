cmake_minimum_required(VERSION 3.2)
project(MessageExtractor CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system program_options filesystem)

set(SOURCE_FILES
        main.cpp)

include_directories(
        ../
        ../../src/LibForumHelpers
        ../../src/LibForumPersistence
        ${Boost_INCLUDE_DIRS})

add_executable(MessageExtractor ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(MessageExtractor
        ${Boost_LIBRARIES})
