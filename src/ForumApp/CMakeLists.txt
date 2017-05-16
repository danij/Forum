cmake_minimum_required(VERSION 3.3)
project(ForumApp CXX)

set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED COMPONENTS log program_options system)
find_package(ICU)

set(SOURCE_FILES
        app.cpp
        Application.cpp
        DefaultIOServiceProvider.cpp)

set(HEADER_FILES
        Application.h
        DefaultIOServiceProvider.h
        ../Logging/Logging.h)

include_directories(
        ../
        ../LibFastJsonWriter
        ../LibHttp
        ../LibForumContext
        ../LibForumServiceEndpoints
        ../LibForumData
        ../LibForumData/private
        ../LibForumHelpers
        ../LibForumPersistence
        ../Logging
        ${Boost_INCLUDE_DIRS})

add_executable(ForumApp
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(ForumApp
        Http
        ForumServiceEndpoints
        ForumData
        ForumHelpers
        ForumPersistence
        ${Boost_LIBRARIES}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES})
