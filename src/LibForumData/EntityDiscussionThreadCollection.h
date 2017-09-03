#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThread.h"
#include "TypeHelpers.h"

#include <functional>
#include <map>

#include <boost/noncopyable.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Forum
{
    namespace Entities
    {
        class DiscussionThreadCollectionBase
        {
        public:
            DECLARE_ABSTRACT_MANDATORY(DiscussionThreadCollectionBase)

            virtual bool add(DiscussionThreadPtr thread);
            virtual bool remove(DiscussionThreadPtr thread);

            void prepareUpdateName(DiscussionThreadPtr thread);
            void updateName(DiscussionThreadPtr thread);

            virtual void stopBatchInsert();

            void prepareUpdateLastUpdated(DiscussionThreadPtr thread);
            void updateLastUpdated(DiscussionThreadPtr thread);

            void prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread);
            void updateLatestMessageCreated(DiscussionThreadPtr thread);

            void prepareUpdateMessageCount(DiscussionThreadPtr thread);
            void updateMessageCount(DiscussionThreadPtr thread);

            virtual void prepareUpdatePinDisplayOrder(DiscussionThreadPtr thread) {} //empty, only used in subclass
            virtual void updatePinDisplayOrder(DiscussionThreadPtr thread) {} //empty, only used in subclass

            auto& onPrepareCountChange()        { return onPrepareCountChange_; }
            auto& onCountChange()               { return onCountChange_; }
                                     
            auto count()                  const { return byName_.size(); }
                                          
            auto byName()                 const { return Helpers::toConst(byName_); }
            auto byCreated()              const { return Helpers::toConst(byCreated_); }
            auto byLastUpdated()          const { return Helpers::toConst(byLastUpdated_); }
            auto byLatestMessageCreated() const { return Helpers::toConst(byLatestMessageCreated_); }
            auto byMessageCount()         const { return Helpers::toConst(byMessageCount_); }

            auto& byName()                 { return byName_; }
            auto& byCreated()              { return byCreated_; }
            auto& byLastUpdated()          { return byLastUpdated_; }
            auto& byLatestMessageCreated() { return byLatestMessageCreated_; }
            auto& byMessageCount()         { return byMessageCount_; }
            
        private:

            RANKED_COLLECTION(DiscussionThread, name) byName_;
            decltype(byName_)::nth_index<0>::type::iterator byNameUpdateIt_;

            RANKED_COLLECTION(DiscussionThread, created) byCreated_;

            RANKED_COLLECTION(DiscussionThread, lastUpdated) byLastUpdated_;
            decltype(byLastUpdated_)::nth_index<0>::type::iterator byLastUpdatedUpdateIt_;

            RANKED_COLLECTION(DiscussionThread, latestMessageCreated) byLatestMessageCreated_;
            decltype(byLatestMessageCreated_)::nth_index<0>::type::iterator byLatestMessageCreatedUpdateIt_;

            RANKED_COLLECTION(DiscussionThread, messageCount) byMessageCount_;
            decltype(byMessageCount_)::nth_index<0>::type::iterator byMessageCountUpdateIt_;

            std::function<void()> onPrepareCountChange_;
            std::function<void()> onCountChange_;
        };

        class DiscussionThreadCollectionWithHashedId : public DiscussionThreadCollectionBase,
                                                       private boost::noncopyable
        {
        public:
            bool add(DiscussionThreadPtr thread) override;
            bool remove(DiscussionThreadPtr thread) override;

            bool contains(DiscussionThreadPtr thread) const;

             auto byId() const { return Helpers::toConst(byId_); }
            auto& byId()       { return byId_; }

        private:
            HASHED_UNIQUE_COLLECTION(DiscussionThread, id) byId_;
        };

        class DiscussionThreadCollectionWithHashedIdAndPinOrder final : public DiscussionThreadCollectionWithHashedId
        {
        public:
            bool add(DiscussionThreadPtr thread) override;
            bool remove(DiscussionThreadPtr thread) override;

            void stopBatchInsert() override;

            void prepareUpdatePinDisplayOrder(DiscussionThreadPtr thread) override;
            void updatePinDisplayOrder(DiscussionThreadPtr thread) override;

            auto  byPinDisplayOrder() const { return Helpers::toConst(byPinDisplayOrder_); }
            auto& byPinDisplayOrder()       { return byPinDisplayOrder_; }

        private:
            ORDERED_COLLECTION(DiscussionThread, pinDisplayOrder) byPinDisplayOrder_;
            decltype(byPinDisplayOrder_)::nth_index<0>::type::iterator byPinDisplayOrderUpdateIt_;
        };

        class DiscussionThreadCollectionWithOrderedId final : public DiscussionThreadCollectionBase, 
                                                              private boost::noncopyable
        {
        public:
            bool add(DiscussionThreadPtr thread) override;
            bool remove(DiscussionThreadPtr thread) override;

            bool contains(DiscussionThreadPtr thread) const;

             auto byId() const { return Helpers::toConst(byId_); }
            auto& byId()       { return byId_; }

        private:
            ORDERED_UNIQUE_COLLECTION(DiscussionThread, id) byId_;
        };

        class DiscussionThreadCollectionWithReferenceCountAndMessageCount final : private boost::noncopyable
        {
        public:
            bool add(DiscussionThreadPtr thread);

            /**
            * Reduces the reference count of the thread, removing it once the count drops to 0
            * Used when a thread is no longer referenced via a tag
            */
            void decreaseReferenceCount(DiscussionThreadPtr thread);

            /**
            * Removes a thread completely, even if the reference count is > 1
            * Used when a thread is permanently deleted
            */
            bool remove(DiscussionThreadPtr thread);

            void clear();
            
            void prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread);
            void updateLatestMessageCreated(DiscussionThreadPtr thread);

            auto count()        const { return byId_.size(); }
            auto messageCount() const { return messageCount_; }
            auto byId()         const { return Helpers::toConst(byId_); }

            auto& byLatestMessageCreated() { return byLatestMessageCreated_; }
            auto& messageCount()           { return messageCount_; }

            DiscussionThreadMessagePtr latestMessage() const;

        private:
            HASHED_UNIQUE_COLLECTION(DiscussionThread, id) byId_;
            
            RANKED_COLLECTION(DiscussionThread, latestMessageCreated) byLatestMessageCreated_;
            decltype(byLatestMessageCreated_)::nth_index<0>::type::iterator byLatestMessageCreatedUpdateIt_;

            int_fast32_t messageCount_ = 0;
            std::map<DiscussionThreadPtr, int_fast32_t> referenceCount_;
        };
    }
}
