cmake_minimum_required(VERSION 3.2)
project(ForumSearchUpdatePlugin CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS log thread filesystem)

set(SOURCE_FILES
        ForumSearchUpdatePlugin.cpp)

set(HEADER_FILES
        ForumSearchUpdatePlugin.h)

include_directories(
        .
        ../../src
        ../../src/LibFastJsonWriter
        ../../src/LibForumData
        ../../src/LibForumHelpers
        ${Boost_INCLUDE_DIRS})

add_library(ForumSearchUpdatePlugin SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(ForumSearchUpdatePlugin
        ForumData
        ForumHelpers
        dl
        atomic
        ${Boost_LIBRARIES})