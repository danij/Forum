cmake_minimum_required(VERSION 3.2)
project(LibHttp CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system)

set(SOURCE_FILES
        private/ConnectionManagerWithTimeout.cpp
        private/FixedHttpConnectionManager.cpp
        private/HttpConnection.cpp
        private/HttpConstants.cpp
        private/HttpParser.cpp
        private/HttpResponseBuilder.cpp
        private/HttpRouter.cpp
        private/StreamingConnection.cpp
        private/TcpListener.cpp)

set(HEADER_FILES
        ConnectionManagerWithTimeout.h
        FixedHttpConnectionManager.h
        FixedSizeBufferPool.h
        FixedSizeObjectPool.h
        HttpConnection.h
        HttpConstants.h
        HttpParser.h
        HttpRequest.h
        HttpResponseBuilder.h
        HttpRouter.h
        HttpStringHelpers.h
        IConnectionManager.h
        ReadWriteBufferArray.h
        StreamingConnection.h
        TcpListener.h
        TimeoutManager.h
        Trie.h)

include_directories(
        .
        ../.
        ${Boost_INCLUDE_DIRS})

add_library(Http SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(Http
        ${Boost_LIBRARIES})
