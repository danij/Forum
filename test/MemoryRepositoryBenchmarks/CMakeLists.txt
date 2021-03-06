cmake_minimum_required(VERSION 3.2)
project(MemoryRepositoryBenchmarks CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system program_options)

find_library(LIB_ATOMIC atomic)
if (LIB_ATOMIC)
    set(ATOMIC_LIBRARIES atomic)
endif()

set(SOURCE_FILES
        benchmarks.cpp)

include_directories(
        ../
        ../../src/LibFastJsonWriter
        ../../src/LibForumContext
        ../../src/LibForumServiceEndpoints
        ../../src/LibForumData
        ../../src/LibForumData/private
        ../../src/LibForumHelpers
        ../../src/LibForumPersistence
        ${Boost_INCLUDE_DIRS})

add_executable(MemoryRepositoryBenchmarks ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(MemoryRepositoryBenchmarks
        ForumServiceEndpoints
        ForumContext
        ForumData
        ForumHelpers
        ForumPersistence
        ${ATOMIC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES})
