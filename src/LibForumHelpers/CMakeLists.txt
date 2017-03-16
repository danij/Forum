cmake_minimum_required(VERSION 3.3)
project(LibForumHelpers CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS locale regex thread system)
find_package(ICU)

set(SOURCE_FILES
        private/RandomGenerator.cpp
        private/StringHelpers.cpp
        private/UuidString.cpp
        private/IpAddress.cpp)

set(HEADER_FILES
        OutputHelpers.h
        RandomGenerator.h
        StateHelpers.h
        StringHelpers.h
        TypeHelpers.h
        UuidString.h
        IpAddress.cpp)

include_directories(
        .
        ../.
        ../LibFastJsonWriter
        ${Boost_INCLUDE_DIRS})

add_library(ForumHelpers SHARED
        ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(ForumHelpers
        FastJsonWriter
        ${Boost_LIBRARIES}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES})
