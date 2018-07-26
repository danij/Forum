cmake_minimum_required(VERSION 3.2)
project(LibForumPersistence CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED COMPONENTS log filesystem)

set(SOURCE_FILES
        private/EventImporter.cpp
        private/EventObserver.cpp
        private/FileAppender.cpp)

set(HEADER_FILES
        EventImporter.h
        EventObserver.h
        PersistenceFormat.h
        private/FileAppender.h
        private/PersistenceBlob.h)

include_directories(
        .
        ../.
        ../LibFastJsonWriter
        ../LibForumContext
        ../LibForumData
        ../LibForumHelpers
        ../Logging
        ${Boost_INCLUDE_DIRS})

add_library(ForumPersistence SHARED
        ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(ForumPersistence
        ForumData
        ${Boost_LIBRARIES})
