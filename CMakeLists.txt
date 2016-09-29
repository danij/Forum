cmake_minimum_required(VERSION 3.3)
project(Forum CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Werror")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++14 -Wall -Werror -O4")

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

add_subdirectory(src/LibFastJsonWriter)
add_subdirectory(src/LibForumContext)
add_subdirectory(src/LibForumCommandHandlers)
add_subdirectory(src/LibForumData)
add_subdirectory(test/ForumServiceTests)

#set(SOURCE_FILES main.cpp)
#add_executable(Forum ${SOURCE_FILES})