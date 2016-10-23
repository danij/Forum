cmake_minimum_required(VERSION 3.3)
project(LibFastJsonWriter CXX)

set(SOURCE_FILES JsonWriter.cpp)
set(HEADER_FILES JsonWriter.h)
add_library(FastJsonWriter STATIC ${SOURCE_FILES} ${HEADER_FILES})
