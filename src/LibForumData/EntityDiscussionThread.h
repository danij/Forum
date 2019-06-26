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

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadMessageCollection.h"
#include "StringHelpers.h"

#include <atomic>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace Forum::Entities
{
    class User;
    class DiscussionTag;
    class DiscussionCategory;

    /**
    * Stores a discussion thread that contains messages
    * Repositories are responsible for updating the relationships between this message and other entities
    */
    class DiscussionThread final : public Authorization::DiscussionThreadPrivilegeStore,
                                   boost::noncopyable
    {
    public:
        const auto& id()                        const { return id_; }

               auto created()                   const { return created_; }
        const auto& creationDetails()           const { return creationDetails_; }

        const auto& createdBy()                 const { return createdBy_; }

        const auto& name()                      const { return name_; }

        const auto& messages()                  const { return messages_; }
               auto messageCount()              const { return messages_.count(); }

               auto empty()                     const { return messages_.empty(); }
               bool approved()                  const { return 1 == approved_; }
               bool aboutToBeDeleted()          const { return 1 == aboutToBeDeleted_; }

        auto lastUpdated() const
        {
            return lastUpdated_ ? lastUpdated_->at : Timestamp{ 0 };
        }

        const auto& lastUpdatedDetails() const
        {
            static const VisitDetails lastUpdatedDetailsDefault{};
            return lastUpdated_ ? lastUpdated_->details : lastUpdatedDetailsDefault;
        }

        StringView lastUpdatedReason() const
        {
            return lastUpdated_ ? lastUpdated_->reason : StringView{};
        }

        auto lastUpdatedBy() const
        {
            return lastUpdated_ ? static_cast<const User*>(lastUpdated_->by) : nullptr;
        }

               auto latestVisibleChange()       const { return latestVisibleChange_; }

               auto latestMessageCreated()      const { return latestMessageCreated_; }

               auto nrOfVisitorsSinceLastEdit() const { return visitorsSinceLastEdit_.size(); }

               auto tags()                      const { return Helpers::toConst(tags_); }
               auto categories()                const { return Helpers::toConst(categories_); }

               auto subscribedUsers()           const { return Helpers::toConst(subscribedUsers_); }
               auto subscribedUsersCount()      const { return subscribedUsers_.size(); }

               auto pinDisplayOrder()           const { return pinDisplayOrder_; }

        DiscussionThreadMessage::VoteScoreType voteScore() const;

        Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(
                Authorization::DiscussionThreadMessagePrivilege privilege) const override;
        Authorization::PrivilegeValueType getDiscussionThreadPrivilege(
                Authorization::DiscussionThreadPrivilege privilege) const override;

        enum ChangeType : uint32_t
        {
            None = 0,
            Name,
            PinDisplayOrder,
            Approval
        };

        typedef Helpers::JsonReadyStringWithSortKey<> NameType;

        struct ChangeNotification final
        {
            std::function<void(DiscussionThread&)> onPrepareUpdateName;
            std::function<void(DiscussionThread&)> onUpdateName;

            std::function<void(DiscussionThread&)> onPrepareUpdateLastUpdated;
            std::function<void(DiscussionThread&)> onUpdateLastUpdated;

            std::function<void(DiscussionThread&)> onPrepareUpdateLatestMessageCreated;
            std::function<void(DiscussionThread&)> onUpdateLatestMessageCreated;

            std::function<void(DiscussionThread&)> onPrepareUpdateMessageCount;
            std::function<void(DiscussionThread&)> onUpdateMessageCount;

            std::function<void(DiscussionThread&)> onPrepareUpdatePinDisplayOrder;
            std::function<void(DiscussionThread&)> onUpdatePinDisplayOrder;
        };

        static auto& changeNotifications() { return changeNotifications_; }

        DiscussionThread(IdType id, User& createdBy, NameType&& name, Timestamp created, VisitDetails creationDetails,
                         Authorization::ForumWidePrivilegeStore& forumWidePrivileges, const bool approved)
            : id_(id), created_(created), creationDetails_(creationDetails),
              createdBy_(createdBy), name_(std::move(name)), forumWidePrivileges_(forumWidePrivileges)
        {
            messages_.onPrepareCountChange()
                = [](void* state) { changeNotifications_.onPrepareUpdateMessageCount(*reinterpret_cast<DiscussionThread*>(state)); };
            messages_.onPrepareCountChange().state() = this;
            messages_.onCountChange()
                = [](void* state) { changeNotifications_.onUpdateMessageCount(*reinterpret_cast<DiscussionThread*>(state)); };
            messages_.onCountChange().state() = this;

            approved_ = approved ? 1 : 0;
            aboutToBeDeleted_ = 0;
            pinDisplayOrder_ = 0;
        }

        void updateName(NameType&& name)
        {
            changeNotifications_.onPrepareUpdateName(*this);
            name_ = std::move(name);
            changeNotifications_.onUpdateName(*this);
        }

        void updateLastUpdated(const Timestamp value)
        {
            if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
            if (lastUpdated_->at == value) return;

            changeNotifications_.onPrepareUpdateLastUpdated(*this);
            lastUpdated_->at = value;
            changeNotifications_.onUpdateLastUpdated(*this);
        }

        void updateLastUpdatedDetails(const VisitDetails& details)
        {
            if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
            lastUpdated_->details = details;
        }

        void updateLastUpdatedReason(std::string&& reason)
        {
            if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
            lastUpdated_->reason = std::move(reason);
        }

        void updateLastUpdatedBy(User* const by)
        {
            if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
            lastUpdated_->by = by;
        }

        auto& latestVisibleChange() { return latestVisibleChange_; }

        /**
        * Thread-safe reference to the number of times the thread was visited.
        * Can be updated even for const values as it is not refenced in any index.
        * @return An atomic integer of at least 64-bits
        */
        auto& visited()    const { return visited_; }

        auto& createdBy()        { return createdBy_; }
        auto& messages()         { return messages_; }
        auto& tags()             { return tags_; }
        auto& categories()       { return categories_; }

        auto& subscribedUsers()  { return subscribedUsers_; }

        void setApproved(bool value)
        {
            approved_ = value ? 1 : 0;
        }
        void setAboutToBeDeleted(bool value)
        {
            aboutToBeDeleted_ = value ? 1 : 0;
        }

        void updatePinDisplayOrder(const uint16_t value)
        {
            changeNotifications_.onPrepareUpdatePinDisplayOrder(*this);
            pinDisplayOrder_ = value;
            changeNotifications_.onUpdatePinDisplayOrder(*this);
        }

        void updateLatestMessageCreated(const Timestamp value)
        {
            changeNotifications_.onPrepareUpdateLatestMessageCreated(*this);
            latestMessageCreated_ = value;
            changeNotifications_.onUpdateLatestMessageCreated(*this);
        }

        void insertMessage(DiscussionThreadMessagePtr message);
        void insertMessages(DiscussionThreadMessageCollectionLowMemory& collection);
        void deleteDiscussionThreadMessage(DiscussionThreadMessagePtr message);

        void addVisitorSinceLastEdit(IdTypeRef userId);
        bool hasVisitedSinceLastEdit(IdTypeRef userId) const;
        void resetVisitorsSinceLastEdit();

        bool addTag(DiscussionTag* tag);
        bool removeTag(DiscussionTag* tag);

        bool addCategory(DiscussionCategory* category);
        bool removeCategory(DiscussionCategory* category);

    private:
        void refreshLatestMessageCreated();

        static ChangeNotification changeNotifications_;

        IdType id_;
        Timestamp created_{0};
        VisitDetails creationDetails_;

        User& createdBy_;

        NameType name_;
        DiscussionThreadMessageCollectionLowMemory messages_;

        std::unique_ptr<LastUpdatedInfo> lastUpdated_;

        //store the timestamp of the latest visible change in order to be able to
        //detect when to return a status that nothing has changed since a provided timestamp
        //Note: do not use as index in collection, the indexes would not always be updated
        Timestamp latestVisibleChange_{0};

        //store the timestamp of the latest message in the collection that was created
        //as it's expensive to retrieve it every time
        Timestamp latestMessageCreated_{0};
        uint16_t pinDisplayOrder_ : 14;
        uint16_t aboutToBeDeleted_ : 1;
        uint16_t approved_ : 1;

        mutable std::atomic_int_fast64_t visited_{0};

        boost::container::flat_set<boost::uuids::uuid> visitorsSinceLastEdit_;

        boost::container::flat_set<DiscussionTag*> tags_;
        boost::container::flat_set<DiscussionCategory*> categories_;
        boost::container::flat_set<User*> subscribedUsers_;

        Authorization::ForumWidePrivilegeStore& forumWidePrivileges_;
    };

    typedef DiscussionThread* DiscussionThreadPtr;
    typedef const DiscussionThread* DiscussionThreadConstPtr;
}
