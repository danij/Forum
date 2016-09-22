cmake_minimum_required(VERSION 3.3)
project(LibForumRepository CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")

set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED)

set(SOURCE_FILES MemoryRepository.cpp)
set(HEADER_FILES ../Version.h Entities.h MemoryRepository.h Repository.h ResourceGuard.h OutputHelpers.h)

include_directories(../. ../LibFastJsonWriter ${Boost_INCLUDE_DIRS})

add_library(libForumRepository STATIC ${SOURCE_FILES} ${HEADER_FILES})
