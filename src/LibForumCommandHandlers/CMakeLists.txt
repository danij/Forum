cmake_minimum_required(VERSION 3.3)
project(LibForumCommandHandlers CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED)

set(SOURCE_FILES CommandHandler.cpp)
set(HEADER_FILES CommandHandler.h ../Version.h)

include_directories(../. ../LibFastJsonWriter ../LibForumData ${Boost_INCLUDE_DIRS})

add_library(ForumCommandHandlers STATIC ${SOURCE_FILES} ${HEADER_FILES})
