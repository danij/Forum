cmake_minimum_required(VERSION 3.3)
project(LibForumCommandHandlers CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system)

set(SOURCE_FILES
        private/CommandHandler.cpp)

set(HEADER_FILES
        CommandHandler.h)

include_directories(
        .
        ../.
        ../LibFastJsonWriter
        ../LibForumContext
        ../LibForumHelpers
        ../LibForumData
        ${Boost_INCLUDE_DIRS})

add_library(ForumCommandHandlers SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(ForumCommandHandlers
        ForumHelpers)
