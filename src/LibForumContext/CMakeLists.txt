cmake_minimum_required(VERSION 3.3)
project(LibForumContext CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS)

set(SOURCE_FILES Configuration.cpp MetricsRepository.cpp)
set(HEADER_FILES ../Version.h Configuration.h MetricsRepository.h)

include_directories(../. ../LibFastJsonWriter ../LibForumRepository ${Boost_INCLUDE_DIRS})

add_library(libForumContext STATIC ${SOURCE_FILES} ${HEADER_FILES})
