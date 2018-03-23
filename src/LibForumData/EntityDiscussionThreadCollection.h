/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThread.h"
#include "TypeHelpers.h"

#include <functional>
#include <unordered_map>

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
        class IDiscussionThreadCollection
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadCollection);

            virtual void prepareUpdateName(DiscussionThreadPtr thread) = 0;
            virtual void updateName(DiscussionThreadPtr thread) = 0;

            virtual void prepareUpdateLastUpdated(DiscussionThreadPtr thread) = 0;
            virtual void updateLastUpdated(DiscussionThreadPtr thread) = 0;

            virtual void prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread) = 0;
            virtual void updateLatestMessageCreated(DiscussionThreadPtr thread) = 0;

            virtual void prepareUpdateMessageCount(DiscussionThreadPtr thread) = 0;
            virtual void updateMessageCount(DiscussionThreadPtr thread) = 0;

            virtual void prepareUpdatePinDisplayOrder(DiscussionThreadPtr thread) = 0;
            virtual void updatePinDisplayOrder(DiscussionThreadPtr thread) = 0;
        };

        class DiscussionThreadCollectionBase : public IDiscussionThreadCollection
        {
        public:
            DECLARE_ABSTRACT_MANDATORY(DiscussionThreadCollectionBase)

            virtual bool add(DiscussionThreadPtr thread);
            virtual bool remove(DiscussionThreadPtr thread);

            virtual void stopBatchInsert();

            void prepareUpdateName(DiscussionThreadPtr thread) override;
            void updateName(DiscussionThreadPtr thread) override;

            void prepareUpdateLastUpdated(DiscussionThreadPtr thread) override;
            void updateLastUpdated(DiscussionThreadPtr thread) override;

            void prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread) override;
            void updateLatestMessageCreated(DiscussionThreadPtr thread) override;

            void prepareUpdateMessageCount(DiscussionThreadPtr thread) override;
            void updateMessageCount(DiscussionThreadPtr thread) override;

            void prepareUpdatePinDisplayOrder(DiscussionThreadPtr thread) override {} //empty, only used in subclass
            void updatePinDisplayOrder(DiscussionThreadPtr thread) override {} //empty, only used in subclass

            auto& onPrepareCountChange()        { return onPrepareCountChange_; }
            auto& onCountChange()               { return onCountChange_; }

            auto count()                  const { return countInternal(); }

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

        protected:
            virtual void iterateAllThreads(std::function<void(DiscussionThreadPtr)>&& callback) = 0;
            virtual size_t countInternal() const = 0;
            void prepareCountChange();
            void finishCountChange();

        private:

            RANKED_COLLECTION(DiscussionThread, name) byName_;
            RANKED_COLLECTION_ITERATOR(byName_) byNameUpdateIt_;

            RANKED_COLLECTION(DiscussionThread, created) byCreated_;

            RANKED_COLLECTION(DiscussionThread, lastUpdated) byLastUpdated_;
            RANKED_COLLECTION_ITERATOR(byLastUpdated_) byLastUpdatedUpdateIt_;

            RANKED_COLLECTION(DiscussionThread, latestMessageCreated) byLatestMessageCreated_;
            RANKED_COLLECTION_ITERATOR(byLatestMessageCreated_) byLatestMessageCreatedUpdateIt_;

            RANKED_COLLECTION(DiscussionThread, messageCount) byMessageCount_;
            RANKED_COLLECTION_ITERATOR(byMessageCount_) byMessageCountUpdateIt_;

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

        protected:
            void iterateAllThreads(std::function<void(DiscussionThreadPtr)>&& callback) override;
            size_t countInternal() const override { return byId_.size(); }

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
            ORDERED_COLLECTION_ITERATOR(byPinDisplayOrder_) byPinDisplayOrderUpdateIt_;
        };

        class DiscussionThreadCollectionWithReferenceCountAndMessageCount final : private boost::noncopyable
        {
        public:
            bool add(DiscussionThreadPtr thread);
            void add(const DiscussionThreadCollectionWithReferenceCountAndMessageCount& collection);

            /**
            * Reduces the reference count of the thread, removing it once the count drops to 0
            * Used when a thread is no longer referenced via a tag
            */
            void decreaseReferenceCount(DiscussionThreadPtr thread);
            void decreaseReferenceCount(const DiscussionThreadCollectionWithReferenceCountAndMessageCount& collection);

            /**
            * Removes a thread completely, even if the reference count is > 1
            * Used when a thread is permanently deleted
            */
            bool remove(DiscussionThreadPtr thread);

            void clear();

            void stopBatchInsert();

            auto count()        const { return byId_.size(); }
            auto messageCount() const { return messageCount_; }
            auto& byId()              { return byId_; }
            auto  byId()        const { return Helpers::toConst(byId_); }

            auto& messageCount()           { return messageCount_; }

        private:
            bool add(DiscussionThreadPtr thread, int_fast32_t amount);

            HASHED_UNIQUE_COLLECTION(DiscussionThread, id) byId_;

            int_fast32_t messageCount_ = 0;
            std::unordered_map<DiscussionThreadPtr, int_fast32_t> referenceCount_;
        };

        class DiscussionThreadCollectionLowMemory final : public IDiscussionThreadCollection,
                                                          private boost::noncopyable
        {
        public:
            bool add(DiscussionThreadPtr thread);
            bool remove(DiscussionThreadPtr thread);

            bool contains(DiscussionThreadPtr thread) const;

            void stopBatchInsert();

            void prepareUpdateName(DiscussionThreadPtr thread) override;
            void updateName(DiscussionThreadPtr thread) override;

            void prepareUpdateLastUpdated(DiscussionThreadPtr thread) override;
            void updateLastUpdated(DiscussionThreadPtr thread) override;

            void prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread) override;
            void updateLatestMessageCreated(DiscussionThreadPtr thread) override;

            void prepareUpdateMessageCount(DiscussionThreadPtr thread) override;
            void updateMessageCount(DiscussionThreadPtr thread) override;

            void prepareUpdatePinDisplayOrder(DiscussionThreadPtr thread) override {} //unused
            void updatePinDisplayOrder(DiscussionThreadPtr thread) override {} //unused

            auto& onPrepareCountChange()        { return onPrepareCountChange_; }
            auto& onCountChange()               { return onCountChange_; }

            auto count()                  const { return byId_.size(); }

            auto byId()                   const { return Helpers::toConst(byId_); }
            auto byName()                 const { return Helpers::toConst(byName_); }
            auto byCreated()              const { return Helpers::toConst(byCreated_); }
            auto byLastUpdated()          const { return Helpers::toConst(byLastUpdated_); }
            auto byLatestMessageCreated() const { return Helpers::toConst(byLatestMessageCreated_); }
            auto byMessageCount()         const { return Helpers::toConst(byMessageCount_); }

            auto& byId()                   { return byId_; }
            auto& byName()                 { return byName_; }
            auto& byCreated()              { return byCreated_; }
            auto& byLastUpdated()          { return byLastUpdated_; }
            auto& byLatestMessageCreated() { return byLatestMessageCreated_; }
            auto& byMessageCount()         { return byMessageCount_; }

        protected:
            void prepareCountChange();
            void finishCountChange();

        private:
            SORTED_VECTOR_UNIQUE_COLLECTION(DiscussionThread, id) byId_;

            SORTED_VECTOR_COLLECTION(DiscussionThread, name) byName_;
            SORTED_VECTOR_COLLECTION_ITERATOR(byName_) byNameUpdateIt_;

            SORTED_VECTOR_COLLECTION(DiscussionThread, created) byCreated_;

            SORTED_VECTOR_COLLECTION(DiscussionThread, lastUpdated) byLastUpdated_;
            SORTED_VECTOR_COLLECTION_ITERATOR(byLastUpdated_) byLastUpdatedUpdateIt_;

            SORTED_VECTOR_COLLECTION(DiscussionThread, latestMessageCreated) byLatestMessageCreated_;
            SORTED_VECTOR_COLLECTION_ITERATOR(byLatestMessageCreated_) byLatestMessageCreatedUpdateIt_;

            SORTED_VECTOR_COLLECTION(DiscussionThread, messageCount) byMessageCount_;
            SORTED_VECTOR_COLLECTION_ITERATOR(byMessageCount_) byMessageCountUpdateIt_;

            std::function<void()> onPrepareCountChange_;
            std::function<void()> onCountChange_;
        };
    }
}
