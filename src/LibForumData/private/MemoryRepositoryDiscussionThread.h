#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionThread final : public MemoryRepositoryBase, public IDiscussionThreadRepository
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
            StatusCode getDiscussionThreadById(const Entities::IdType& id, OutStream& output) override;

            StatusCode getDiscussionThreadsOfUser(const Entities::IdType& id, OutStream& output,
                                                  RetrieveDiscussionThreadsBy by) const override;
            StatusCode getSubscribedDiscussionThreadsOfUser(const Entities::IdType& id, OutStream& output,
                                                            RetrieveDiscussionThreadsBy by) const override;

            StatusCode getDiscussionThreadsWithTag(const Entities::IdType& id, OutStream& output,
                                                   RetrieveDiscussionThreadsBy by) const override;

            StatusCode getDiscussionThreadsOfCategory(const Entities::IdType& id, OutStream& output,
                                                      RetrieveDiscussionThreadsBy by) const override;

            StatusCode addNewDiscussionThread(StringView name, OutStream& output) override;
            StatusCode changeDiscussionThreadName(const Entities::IdType& id, StringView newName,
                                                  OutStream& output) override;
            StatusCode changeDiscussionThreadPinDisplayOrder(const Entities::IdType& id, uint16_t newValue,
                                                             OutStream& output) override;
            StatusCode deleteDiscussionThread(const Entities::IdType& id, OutStream& output) override;
            StatusCode mergeDiscussionThreads(const Entities::IdType& fromId, const Entities::IdType& intoId, 
                                              OutStream& output) override;
            StatusCode subscribeToDiscussionThread(const Entities::IdType& id, OutStream& output) override;
            StatusCode unsubscribeFromDiscussionThread(const Entities::IdType& id, OutStream& output) override;

        private:
            Authorization::DiscussionThreadAuthorizationRef authorization_;
        };
    }
}
