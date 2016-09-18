cmake_minimum_required(VERSION 3.3)
project(Forum CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")

add_subdirectory(src/LibFastJsonWriter)
add_subdirectory(src/LibForumCommandHandlers)
add_subdirectory(test/ForumServiceTests)

#set(SOURCE_FILES main.cpp)
#add_executable(Forum ${SOURCE_FILES})