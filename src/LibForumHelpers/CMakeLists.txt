cmake_minimum_required(VERSION 3.2)
project(LibForumHelpers CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS regex thread system)
find_package(ICU)

set(SOURCE_FILES
        private/StringHelpers.cpp
        private/IpAddress.cpp
        private/RandomGenerator.cpp
        private/UuidString.cpp)

set(HEADER_FILES
        ConstCollectionAdapter.h
        OutputHelpers.h
        StateHelpers.h
        StringHelpers.h
        TypeHelpers.h
        IpAddress.h
        RandomGenerator.h
        SeparateThreadConsumer.h
        SpinLock.h
        UuidString.h)

include_directories(
        .
        ../.
        ../LibFastJsonWriter
        ${Boost_INCLUDE_DIRS})

add_library(ForumHelpers SHARED
        ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(ForumHelpers
        ${Boost_LIBRARIES}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES})
