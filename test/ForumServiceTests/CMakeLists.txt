cmake_minimum_required(VERSION 3.3)
project(ForumServiceTests CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")

set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS unit_test_framework locale regex)
find_package(ICU)

set(SOURCE_FILES main.cpp SimpleCommandTests.cpp CommandsCommon.cpp UserTests.cpp)
set(HEADER_FILES TestHelpers.h CommandsCommon.h)

include_directories(../../src ../../src/LibFastJsonWriter
        ../../src/LibForumCommandHandlers ../../src/LibForumRepository ${Boost_INCLUDE_DIRS})

add_executable(ForumServiceTests ${SOURCE_FILES} ${HEADER_FILES} ${JSON_HEADER_FILES})
target_link_libraries(ForumServiceTests libFastJsonWriter libForumCommandHandlers libForumRepository
        ${Boost_LIBRARIES} ${ICU_LIBRARIES} ${ICU_I18N_LIBRARIES})
install (TARGETS ForumServiceTests RUNTIME DESTINATION ${PROJECT_BINARY_DIR})
