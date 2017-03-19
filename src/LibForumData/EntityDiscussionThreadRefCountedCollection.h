#pragma once

#include "EntityDiscussionThread.h"
#include "EntityDiscussionThreadCollectionBase.h"
#include "TypeHelpers.h"

#include <memory>

namespace Forum
{
    namespace Entities
    {
        /**
         * Used by discussion categories to hold references to all discussion threads, including those of children
         */
        template<typename IndexTypeForId>
        struct DiscussionThreadRefCountedCollection : public DiscussionThreadCollectionBase<IndexTypeForId>
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(DiscussionThreadRefCountedCollection);

            typedef typename DiscussionThreadCollectionBase<IndexTypeForId>::ThreadIdIteratorType ThreadIdIteratorType;

            int_fast32_t  messageCount() const { return messageCount_; }
            int_fast32_t& messageCount()       { return messageCount_; }

            const DiscussionThreadMessage* latestMessage() const
            {
                auto index = DiscussionThreadCollectionBase<IndexTypeForId>::threadsByLatestMessageCreated();
                if ( ! index.size())
                {
                    return nullptr;
                }
                auto thread = *(index.rbegin());
                auto messageIndex = thread->messagesByCreated();
                if (messageIndex.size())
                {
                    return *(messageIndex.rbegin());
                }
                return nullptr;                
            }

            /**
             * Reduces the reference count of the thread, removing it once the count drops to 0
             * Used when a thread is no longer referenced via a tag
             */
            void decreaseReferenceCount(const DiscussionThreadRef& thread)
            {
                auto it = referenceCount_.find(thread);
                if (it == referenceCount_.end())
                {
                    return;
                }
                if ((it->second -= 1) < 1)
                {
                    referenceCount_.erase(it);
                    if (thread)
                    {
                        DiscussionThreadCollectionBase<IndexTypeForId>::deleteDiscussionThreadById(thread->id());
                    }
                }
            }
            
            bool insertDiscussionThread(const DiscussionThreadRef& thread) override
            {
                auto it = referenceCount_.find(thread);
                if (it == referenceCount_.end() 
                    && DiscussionThreadCollectionBase<IndexTypeForId>::insertDiscussionThread(thread))
                {
                    referenceCount_.insert(std::make_pair(thread, 1));
                    messageCount_ += thread->messagesById().size();
                    return true;
                }
                it->second += 1;
                return false;
            }

            /**
             * Removes a thread completely, even if the reference count is > 1 
             * Used when a thread is permanently deleted
             */
            DiscussionThreadRef deleteDiscussionThread(ThreadIdIteratorType iterator) override
            {
                DiscussionThreadRef result;
                if ( ! ((result = DiscussionThreadCollectionBase<IndexTypeForId>::deleteDiscussionThread(iterator))))
                {
                    return result;
                }
                referenceCount_.erase(result);
                if (result)
                {
                    messageCount_ -= result->messages().size();
                }
                return result;
            }

        protected:
            int_fast32_t messageCount_ = 0;
            std::map<DiscussionThreadRef, int_fast32_t, std::owner_less<DiscussionThreadRef>> referenceCount_;
        };
    }
}
