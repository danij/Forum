cmake_minimum_required(VERSION 3.3)
project(LibForumCommandHandlers)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")

set(SOURCE_FILES VersionHandler.cpp)
set(HEADER_FILES CommandHandlers.h ../Version.h)

include_directories(../. ../LibFastJsonWriter)

add_library(libForumCommandHandlers STATIC ${SOURCE_FILES} ${HEADER_FILES})
