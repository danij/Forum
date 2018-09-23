cmake_minimum_required(VERSION 3.2)
project(LibForumServiceEndpoints CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system)

set(SOURCE_FILES
        private/CommandHandler.cpp
        private/ServiceEndpointManager.cpp
        private/ServiceEndpoints.cpp)

set(HEADER_FILES
        CommandHandler.h
        ServiceEndpointManager.h
        private/AuthStore.h
        private/ServiceEndpoints.h)

include_directories(
        .
        ../.
        ../LibFastJsonWriter
        ../LibHttp
        ../LibForumContext
        ../LibForumHelpers
        ../LibForumData
        ${Boost_INCLUDE_DIRS})

add_library(ForumServiceEndpoints SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(ForumServiceEndpoints
        Http
        ForumContext
        ForumHelpers
        ForumData
        ${Boost_LIBRARIES})
