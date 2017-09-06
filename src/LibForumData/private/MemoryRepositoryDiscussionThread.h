#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionThread final : public MemoryRepositoryBase,
                                                       public IDiscussionThreadRepository,
                                                       public IDiscussionThreadDirectWriteRepository
        {
        public:
            explicit MemoryRepositoryDiscussionThread(MemoryStoreRef store,
                                                      Authorization::DiscussionThreadAuthorizationRef authorization);

            StatusCode getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const override;

            /**
             * Calling the function changes state:
             * - Increases the number of visits
             * - Stores that the current user has visited the discussion thread
             */
            StatusCode getDiscussionThreadById(Entities::IdTypeRef id, OutStream& output) override;

            StatusCode getDiscussionThreadsOfUser(Entities::IdTypeRef id, OutStream& output,
                                                  RetrieveDiscussionThreadsBy by) const override;
            StatusCode getSubscribedDiscussionThreadsOfUser(Entities::IdTypeRef id, OutStream& output,
                                                            RetrieveDiscussionThreadsBy by) const override;

            StatusCode getDiscussionThreadsWithTag(Entities::IdTypeRef id, OutStream& output,
                                                   RetrieveDiscussionThreadsBy by) const override;

            StatusCode getDiscussionThreadsOfCategory(Entities::IdTypeRef id, OutStream& output,
                                                      RetrieveDiscussionThreadsBy by) const override;

            StatusCode addNewDiscussionThread(StringView name, OutStream& output) override;
            StatusWithResource<Entities::DiscussionThreadPtr>
                       addNewDiscussionThread(Entities::EntityCollection& collection, Entities::IdTypeRef id, 
                                              StringView name) override;

            StatusCode changeDiscussionThreadName(Entities::IdTypeRef id, StringView newName,
                                                  OutStream& output) override;
            StatusCode changeDiscussionThreadName(Entities::EntityCollection& collection,
                                                  Entities::IdTypeRef id, StringView newName) override;
            StatusCode changeDiscussionThreadPinDisplayOrder(Entities::IdTypeRef id, uint16_t newValue,
                                                             OutStream& output) override;
            StatusCode changeDiscussionThreadPinDisplayOrder(Entities::EntityCollection& collection,
                                                             Entities::IdTypeRef id, uint16_t newValue) override;
            StatusCode deleteDiscussionThread(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode deleteDiscussionThread(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;
            StatusCode mergeDiscussionThreads(Entities::IdTypeRef fromId, Entities::IdTypeRef intoId,
                                              OutStream& output) override;
            StatusCode mergeDiscussionThreads(Entities::EntityCollection& collection,
                                              Entities::IdTypeRef fromId, Entities::IdTypeRef intoId) override;
            StatusCode subscribeToDiscussionThread(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode subscribeToDiscussionThread(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;
            StatusCode unsubscribeFromDiscussionThread(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode unsubscribeFromDiscussionThread(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;

            StatusCode getRequiredPrivileges(Entities::IdTypeRef threadId, OutStream& output) const override;
            StatusCode getDefaultPrivilegeDurations(Entities::IdTypeRef threadId, OutStream& output) const override;
            StatusCode getAssignedPrivileges(Entities::IdTypeRef threadId, OutStream& output) const override;

            StatusCode changeDiscussionThreadMessageRequiredPrivilegeForThread(
                    Entities::IdTypeRef threadId, Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueIntType value, OutStream& output) override;
            StatusCode changeDiscussionThreadMessageRequiredPrivilegeForThread(
                    Entities::EntityCollection& collection,
                    Entities::IdTypeRef threadId, Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueIntType value) override;
            StatusCode changeDiscussionThreadRequiredPrivilegeForThread(
                    Entities::IdTypeRef threadId, Authorization::DiscussionThreadPrivilege privilege,
                    Authorization::PrivilegeValueIntType value, OutStream& output) override;
            StatusCode changeDiscussionThreadRequiredPrivilegeForThread(
                    Entities::EntityCollection& collection,
                    Entities::IdTypeRef threadId, Authorization::DiscussionThreadPrivilege privilege,
                    Authorization::PrivilegeValueIntType value) override;

            StatusCode changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(
                    Entities::IdTypeRef threadId, Authorization::DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    Authorization::PrivilegeDefaultDurationIntType value, OutStream& output) override;
            StatusCode changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(
                    Entities::EntityCollection& collection,
                    Entities::IdTypeRef threadId, Authorization::DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    Authorization::PrivilegeDefaultDurationIntType value) override;

            StatusCode assignDiscussionThreadMessagePrivilegeForThread(
                    Entities::IdTypeRef threadId, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionThreadMessagePrivilegeForThread(
                    Entities::EntityCollection& collection, Entities::IdTypeRef threadId, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;
            StatusCode assignDiscussionThreadPrivilegeForThread(
                    Entities::IdTypeRef threadId, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadPrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionThreadPrivilegeForThread(
                    Entities::EntityCollection& collection, Entities::IdTypeRef threadId, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadPrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;


        private:
            Authorization::DiscussionThreadAuthorizationRef authorization_;
        };
    }
}
