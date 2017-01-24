cmake_minimum_required(VERSION 3.3)
project(LibForumData CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS locale regex thread system)
find_package(ICU)

set(SOURCE_FILES RandomGenerator.cpp EntitySerialization.cpp StringHelpers.cpp EntityCollection.cpp UuidString.cpp 
        MemoryRepository-main.cpp MemoryRepository-users.cpp MemoryRepository-threads.cpp 
        MemoryRepository-messages.cpp MemoryRepository-tags.cpp)
set(HEADER_FILES Entities.h MemoryRepository.h Repository.h ResourceGuard.h OutputHelpers.h
        ConstCollectionAdapter.h StringHelpers.h RandomGenerator.h
        EntitySerialization.h EntityCollection.h Observers.h EntityCommonTypes.h UuidString.h
        TypeHelpers.h EntityDiscussionThreadCollectionBase.h EntityDiscussionThread.h EntityUser.h StateHelpers.h
        EntityDiscussionThreadMessage.h EntityDiscussionThreadMessageCollectionBase.h EntityUserCollectionBase.h
        EntityDiscussionTag.h EntityDiscussionTagCollectionBase.h)

include_directories(. ../. ../LibFastJsonWriter ../LibForumContext ${Boost_INCLUDE_DIRS})

add_library(ForumData STATIC ${SOURCE_FILES} ${HEADER_FILES})
