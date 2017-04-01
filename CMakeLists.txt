cmake_minimum_required(VERSION 3.3)
project(Forum CXX)
enable_testing()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Werror -Wno-error=deprecated-declarations")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

add_subdirectory(src/LibFastJsonWriter)
add_subdirectory(src/LibForumContext)
add_subdirectory(src/LibForumServiceEndpoints)
add_subdirectory(src/LibForumHelpers)
add_subdirectory(src/LibForumData)
add_subdirectory(test/ForumServiceTests)
add_subdirectory(test/MemoryRepositoryBenchmarks)

#set(SOURCE_FILES main.cpp)
#add_executable(Forum ${SOURCE_FILES})
