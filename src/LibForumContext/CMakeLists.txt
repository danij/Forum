cmake_minimum_required(VERSION 3.2)
project(LibForumContext CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system)

set(SOURCE_FILES
        private/Configuration.cpp
        private/ContextProviders.cpp
        private/DefaultIOServiceProvider.cpp)

set(HEADER_FILES
        Configuration.h
        ContextProviders.h
        ContextProviderMocks.h
        DefaultIOServiceProvider.h)

include_directories(
        .
        ../.
        ../LibFastJsonWriter
        ../LibForumData
        ../LibForumHelpers
        ${Boost_INCLUDE_DIRS})

add_library(ForumContext SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(ForumContext
        FastJsonWriter
        ForumHelpers)
