cmake_minimum_required(VERSION 3.2)
project(LibForumData CXX)

set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS locale log regex system)
find_package(ICU)

set(SOURCE_FILES
        private/EntityPointer.cpp
        private/EntityCollection.cpp
        private/EntityUser.cpp
        private/EntityUserCollection.cpp
        private/EntityDiscussionThread.cpp
        private/EntityDiscussionThreadCollection.cpp
        private/EntityDiscussionThreadMessage.cpp
        private/EntityDiscussionThreadMessageCollection.cpp
        private/EntityDiscussionTag.cpp
        private/EntityDiscussionTagCollection.cpp
        private/EntityDiscussionCategory.cpp
        private/EntityDiscussionCategoryCollection.cpp
        private/EntityMessageCommentCollection.cpp
        private/EntityPrivateMessageCollection.cpp
        private/MemoryRepositoryCommon.cpp
        private/MemoryRepositoryUser.cpp
        private/MemoryRepositoryDiscussionThread.cpp
        private/MemoryRepositoryStatistics.cpp
        private/MemoryRepositoryDiscussionThreadMessage.cpp
        private/MemoryRepositoryDiscussionTag.cpp
        private/MemoryRepositoryDiscussionCategory.cpp
        private/MemoryRepositoryAuthorization.cpp
        private/MetricsRepository.cpp
        private/EntitySerialization.cpp
        private/DefaultAuthorization.cpp
        private/AuthorizationGrantedPrivilegeStore.cpp
        private/DefaultThrottling.cpp
        private/VisitorCollection.cpp)

set(HEADER_FILES
        ../Version.h
        Repository.h
        Authorization.h
        Observers.h
        Entities.h
        EntityCollection.h
        EntityCommonTypes.h
        EntityPointer.h
        VectorWithFreeQueue.h
        EntityUser.h
        EntityUserCollection.h
        EntityDiscussionThread.h
        EntityDiscussionThreadCollection.h
        EntityDiscussionThreadMessage.h
        EntityDiscussionThreadMessageCollection.h
        EntityDiscussionTag.h
        EntityDiscussionTagCollection.h
        EntityDiscussionCategory.h
        EntityDiscussionCategoryCollection.h
        EntityMessageComment.h
        EntityMessageCommentCollection.h        
        EntityPrivateMessage.h
        EntityPrivateMessageCollection.h
        EntitySerialization.h
        DefaultAuthorization.h
        DefaultThrottling.h
        AuthorizationPrivileges.h
        AuthorizationGrantedPrivilegeStore.h
        ThrottlingCheck.h
        SortedVector.h
        IdOrIpAddress.h
        MemoryRepositoryCommon.h
        MemoryRepositoryUser.h
        MemoryRepositoryDiscussionThread.h
        MemoryRepositoryStatistics.h
        MemoryRepositoryDiscussionThreadMessage.h
        MemoryRepositoryDiscussionTag.h
        MemoryRepositoryDiscussionCategory.h
        MemoryRepositoryAuthorization.h
        MetricsRepository.h
        ResourceGuard.h
        VisitorCollection.h)

include_directories(
        .
        ../.
        ../LibFastJsonWriter
        ../LibForumContext
        ../LibForumHelpers
        ../Logging
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
