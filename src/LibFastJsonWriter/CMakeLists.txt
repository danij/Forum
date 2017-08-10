cmake_minimum_required(VERSION 3.2)
project(LibFastJsonWriter CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED)

set(SOURCE_FILES
        private/JsonWriter.cpp)

set(HEADER_FILES
        StringBuffer.h
        JsonWriter.h)

include_directories(
        .
        ../.
        ${Boost_INCLUDE_DIRS})

add_library(FastJsonWriter SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES})
