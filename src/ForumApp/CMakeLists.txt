cmake_minimum_required(VERSION 3.3)
project(ForumApp CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system)
find_package(ICU)

set(SOURCE_FILES
        app.cpp
        Application.cpp
        DefaultIOServiceProvider.cpp)

set(HEADER_FILES
        Application.h
        DefaultIOServiceProvider.h)

include_directories(
        ../../src
        ../../src/LibFastJsonWriter
        ../../src/LibHttp
        ../../src/LibForumContext
        ../../src/LibForumServiceEndpoints
        ../../src/LibForumData
        ../../src/LibForumData/private
        ../../src/LibForumHelpers
        ${Boost_INCLUDE_DIRS})

add_executable(ForumApp
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(ForumApp
        Http
        ForumServiceEndpoints
        ForumData
        ForumHelpers
        ${Boost_LIBRARIES}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES})
