cmake_minimum_required(VERSION 3.3)
project(LibFastJsonWriter CXX)

set(SOURCE_FILES
        private/JsonWriter.cpp)

set(HEADER_FILES
        JsonWriter.h)

include_directories(
        .
        ../.)

add_library(FastJsonWriter SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES})
