cmake_minimum_required(VERSION 3.2)
project(Forum C CXX)
enable_testing()

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    message(FATAL_ERROR "Big endian is currently not supported by this project")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall -Werror -Wno-error=deprecated-declarations")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=maybe-uninitialized -fprofile-arcs -ftest-coverage")
endif()

if (APPLE)
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
    if(NOT CMAKE_BUILD_TYPE MATCHES DEBUG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
    endif()
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    add_definitions(-DDEBUG)
else()
    add_definitions(-DNDEBUG)
endif()

add_definitions(-DBOOST_ASIO_ENABLE_OLD_SERVICES)

add_subdirectory(src/LibFastJsonWriter)
add_subdirectory(src/LibHttp)
add_subdirectory(src/LibForumContext)
add_subdirectory(src/LibForumServiceEndpoints)
add_subdirectory(src/LibForumHelpers)
add_subdirectory(src/LibForumData)
add_subdirectory(src/LibForumPersistence)

add_subdirectory(src/ForumApp)

add_subdirectory(test/ForumServiceTests)
add_subdirectory(test/HttpTests)
add_subdirectory(test/MemoryRepositoryBenchmarks)

add_subdirectory(tools/HttpBenchmark)
add_subdirectory(tools/MessageExtractor)
add_subdirectory(tools/SearchDataExtractor)
