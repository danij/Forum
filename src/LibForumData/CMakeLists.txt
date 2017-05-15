cmake_minimum_required(VERSION 3.3)
project(LibForumData CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS locale regex system)
find_package(ICU)

set(SOURCE_FILES
        private/EntitySerialization.cpp
        private/EntityCollection.cpp
        private/EntityDiscussionThread.cpp
        private/EntityDiscussionCategory.cpp
        private/EntityDiscussionTag.cpp
        private/MemoryRepositoryCommon.cpp
        private/MemoryRepositoryUser.cpp
        private/MemoryRepositoryDiscussionThread.cpp
        private/MemoryRepositoryStatistics.cpp
        private/MemoryRepositoryDiscussionThreadMessage.cpp
        private/MemoryRepositoryDiscussionTag.cpp
        private/MemoryRepositoryDiscussionCategory.cpp
        private/MetricsRepository.cpp
        private/DefaultAuthorization.cpp
        private/AuthorizationGrantedPrivilegeStore.cpp)

set(HEADER_FILES
        ../Version.h
        Repository.h
        Authorization.h
        Observers.h
        Entities.h
        ConstCollectionAdapter.h
        EntityCommonTypes.h
        EntityUser.h
        EntityDiscussionThread.h
        EntityDiscussionThreadMessage.h
        EntityDiscussionThreadMessageCollectionBase.h
        EntityUserCollectionBase.h
        EntityDiscussionThreadCollectionBase.h
        EntityDiscussionTagCollectionBase.h
        EntityMessageComment.h
        EntityDiscussionCategoryCollectionBase.h
        EntityDiscussionThreadRefCountedCollection.h
        EntityMessageCommentCollectionBase.h
        EntityDiscussionTag.h
        EntityDiscussionCategory.h
        DefaultAuthorization.h
        AuthorizationPrivileges.h
        AuthorizationGrantedPrivilegeStore.h

        private/ResourceGuard.h
        private/EntitySerialization.h
        private/EntityCollection.h
        private/MemoryRepositoryCommon.h
        private/MemoryRepositoryUser.h
        private/MemoryRepositoryDiscussionThread.h
        private/MemoryRepositoryStatistics.h
        private/MemoryRepositoryDiscussionThreadMessage.h
        private/MemoryRepositoryDiscussionTag.h
        private/MemoryRepositoryDiscussionCategory.h
        private/MetricsRepository.h)

include_directories(
        .
        ../.
        ../LibFastJsonWriter
        ../LibForumContext
        ../LibForumHelpers
        ${Boost_INCLUDE_DIRS})

add_library(ForumData SHARED
        ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(
        ForumData
        FastJsonWriter
        ForumContext
        ForumHelpers
        ${Boost_LIBRARIES}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES})
