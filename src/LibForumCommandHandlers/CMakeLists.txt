cmake_minimum_required(VERSION 3.3)
project(LibForumCommandHandlers CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")

set(SOURCE_FILES CommandHandler.cpp)
set(HEADER_FILES CommandHandler.h ../Version.h)

include_directories(../. ../LibFastJsonWriter ../LibForumRepository)

add_library(libForumCommandHandlers STATIC ${SOURCE_FILES} ${HEADER_FILES})
