cmake_minimum_required(VERSION 3.3)
project(ForumServiceTests CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS unit_test_framework system)
find_package(ICU)

set(SOURCE_FILES
        main.cpp
        SimpleCommandTests.cpp
        CommandsCommon.cpp
        UserTests.cpp
        DiscussionThreadTests.cpp
        TestHelpers.cpp
        ObserversTests.cpp
        DiscussionTagTests.cpp
        DiscussionCategoryTests.cpp
        JsonSerializationTests.cpp
        IpAddressTests.cpp)

set(HEADER_FILES
        TestHelpers.h
        CommandsCommon.h)

include_directories(
        ../../src
        ../../src/LibFastJsonWriter
        ../../src/LibForumContext
        ../../src/LibForumServiceEndpoints
        ../../src/LibForumData
        ../../src/LibForumData/private
        ../../src/LibForumHelpers
        ${Boost_INCLUDE_DIRS})

add_executable(ForumServiceTests
        ${SOURCE_FILES}
        ${HEADER_FILES})

target_link_libraries(ForumServiceTests
        ForumServiceEndpoints
        ForumData
        ForumHelpers
        ${Boost_LIBRARIES}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES})

add_test(NAME Service_Command_Tests COMMAND ForumServiceTests)
