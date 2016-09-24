cmake_minimum_required(VERSION 3.3)
project(LibForumRepository CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS locale regex thread system)
find_package(ICU)

set(SOURCE_FILES MemoryRepository.cpp RandomGenerator.cpp EntitySerialization.cpp)
set(HEADER_FILES ../Version.h Entities.h MemoryRepository.h Repository.h ResourceGuard.h OutputHelpers.h
        ConstCollectionAdapter.h StringHelpers.h Configuration.h Configuration.cpp RandomGenerator.h
        EntitySerialization.h)

include_directories(../. ../LibFastJsonWriter ${Boost_INCLUDE_DIRS})

add_library(libForumRepository STATIC ${SOURCE_FILES} ${HEADER_FILES})
