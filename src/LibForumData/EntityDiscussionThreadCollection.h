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
#include "ContextProviders.h"
#include "EntityDiscussionThread.h"
#include "TypeHelpers.h"

#include <functional>
#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace Forum::Entities
{
    class IDiscussionThreadCollection
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IDiscussionThreadCollection)

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

    class DiscussionThreadCollectionWithHashedId : public IDiscussionThreadCollection
    {
    public:
        virtual bool add(DiscussionThreadPtr thread);
        virtual bool add(DiscussionThreadPtr* threads, size_t threadCount);
        virtual bool remove(DiscussionThreadPtr thread);

        void stopBatchInsert();

        bool contains(DiscussionThreadPtr thread) const;

        const DiscussionThread* findById(IdTypeRef id) const;
            DiscussionThreadPtr findById(IdTypeRef id);

        template<typename Fn>
        void iterateThreads(Fn&& callback) const
        {
            if (Context::isBatchInsertInProgress())
            {
                for (auto& threadPtr : byId_)
                {
                    callback(static_cast<const DiscussionThread*>(threadPtr));
                }
            }
            else
            {
                for (auto threadPtr : byCreated_)
                {
                    callback(static_cast<const DiscussionThread*>(threadPtr));
                }
            }
        }

        template<typename Fn>
        void iterateThreads(Fn&& callback)
        {
            if (Context::isBatchInsertInProgress())
            {
                for (auto& threadPtr : byId_)
                {
                    callback(threadPtr);
                }
            }
            else
            {
                for (auto threadPtr : byCreated_)
                {
                    callback(threadPtr);
                }
            }
        }

        void prepareUpdateName(DiscussionThreadPtr thread) override;
        void updateName(DiscussionThreadPtr thread) override;

        void prepareUpdateLastUpdated(DiscussionThreadPtr thread) override;
        void updateLastUpdated(DiscussionThreadPtr thread) override;

        void prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread) override;
        void updateLatestMessageCreated(DiscussionThreadPtr thread) override;

        void prepareUpdateMessageCount(DiscussionThreadPtr thread) override;
        void updateMessageCount(DiscussionThreadPtr thread) override;

        void prepareUpdatePinDisplayOrder(DiscussionThreadPtr /*thread*/) override {} //empty, only used in subclass
        void updatePinDisplayOrder(DiscussionThreadPtr /*thread*/) override {} //empty, only used in subclass

        auto& onPrepareCountChange()        { return onPrepareCountChange_; }
        auto& onCountChange()               { return onCountChange_; }

        auto count()                  const { return byId_.size(); }

        auto byName()                 const { return Helpers::toConst(byName_); }
        auto byCreated()              const { return Helpers::toConst(byCreated_); }
        auto byLastUpdated()          const { return Helpers::toConst(byLastUpdated_); }
        auto byLatestMessageCreated() const { return Helpers::toConst(byLatestMessageCreated_); }
        auto byMessageCount()         const { return Helpers::toConst(byMessageCount_); }

    protected:
        void prepareCountChange();
        void finishCountChange();

        virtual void onStopBatchInsert();

        HASHED_UNIQUE_COLLECTION(DiscussionThread, id) byId_;

    private:
        RANKED_COLLECTION(DiscussionThread, name) byName_;
        RANKED_COLLECTION_ITERATOR(byName_) byNameUpdateIt_;

        SORTED_VECTOR_COLLECTION(DiscussionThread, created) byCreated_;

        RANKED_COLLECTION(DiscussionThread, lastUpdated) byLastUpdated_;
        RANKED_COLLECTION_ITERATOR(byLastUpdated_) byLastUpdatedUpdateIt_;

        RANKED_COLLECTION(DiscussionThread, latestMessageCreated) byLatestMessageCreated_;
        RANKED_COLLECTION_ITERATOR(byLatestMessageCreated_) byLatestMessageCreatedUpdateIt_;

        SORTED_VECTOR_COLLECTION(DiscussionThread, messageCount) byMessageCount_;
        SORTED_VECTOR_COLLECTION_ITERATOR(byMessageCount_) byMessageCountUpdateIt_;

        std::function<void()> onPrepareCountChange_;
        std::function<void()> onCountChange_;
    };

    class DiscussionThreadCollectionWithHashedIdAndPinOrder final : public DiscussionThreadCollectionWithHashedId
    {
    public:
        bool add(DiscussionThreadPtr thread) override;
        bool add(DiscussionThreadPtr* threads, size_t threadCount) override;
        bool remove(DiscussionThreadPtr thread) override;

        void onStopBatchInsert() override;

        void prepareUpdatePinDisplayOrder(DiscussionThreadPtr thread) override;
        void updatePinDisplayOrder(DiscussionThreadPtr thread) override;

        auto byPinDisplayOrder() const { return Helpers::toConst(byPinDisplayOrder_); }

    private:
        SORTED_VECTOR_COLLECTION(DiscussionThread, pinDisplayOrder) byPinDisplayOrder_;
        SORTED_VECTOR_COLLECTION_ITERATOR(byPinDisplayOrder_) byPinDisplayOrderUpdateIt_;
    };

    class DiscussionThreadCollectionWithReferenceCountAndMessageCount final : boost::noncopyable
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
                                                      boost::noncopyable
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

        void prepareUpdatePinDisplayOrder(DiscussionThreadPtr /*thread*/) override {} //unused
        void updatePinDisplayOrder(DiscussionThreadPtr /*thread*/) override {} //unused

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
