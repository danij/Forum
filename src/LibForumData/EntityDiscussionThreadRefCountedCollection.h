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
        struct DiscussionThreadRefCountedCollection : public DiscussionThreadCollectionBase
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(DiscussionThreadRefCountedCollection);

            int_fast32_t  messageCount() const { return messageCount_; }
            int_fast32_t& messageCount()       { return messageCount_; }

            const DiscussionThreadMessage* latestMessage() const;

            /**
             * Reduces the reference count of the thread, removing it once the count drops to 0
             * Used when a thread is no longer referenced via a tag
             */
            void decreaseReferenceCount(const DiscussionThreadRef& thread);
            
            bool insertDiscussionThread(const DiscussionThreadRef& thread) override;

            /**
             * Removes a thread completely, even if the reference count is > 1 
             * Used when a thread is permanently deleted
             */
            DiscussionThreadRef deleteDiscussionThread(DiscussionThreadCollection::iterator iterator) override;

        protected:
            int_fast32_t messageCount_ = 0;
            std::map<DiscussionThreadRef, int_fast32_t, std::owner_less<DiscussionThreadRef>> referenceCount_;
        };
    }
}
