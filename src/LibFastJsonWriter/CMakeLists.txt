cmake_minimum_required(VERSION 3.3)
project(LibFastJsonWriter CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")

set(SOURCE_FILES JsonWriter.cpp)
set(HEADER_FILES JsonWriter.h)
add_library(libFastJsonWriter STATIC ${SOURCE_FILES} ${HEADER_FILES})
