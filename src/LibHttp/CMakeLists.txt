cmake_minimum_required(VERSION 3.2)
project(LibHttp CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system)

set(SOURCE_FILES
        private/HttpConstants.cpp
        private/HttpListener.cpp
        private/HttpParser.cpp
        private/HttpResponseBuilder.cpp
        private/HttpRouter.cpp)

set(HEADER_FILES
        HttpConstants.h
        HttpListener.h
        HttpRequest.h
        HttpResponseBuilder.h
        HttpRouter.h
        HttpStringHelpers.h
        private/FixedSizeBuffer.h
        private/HttpParser.h
        private/TimeoutManager.h)

include_directories(
        .
        ../.
        ${Boost_INCLUDE_DIRS})

add_library(Http SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(Http
        ${Boost_LIBRARIES})
