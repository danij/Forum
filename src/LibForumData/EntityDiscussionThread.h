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
#include <set>
#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Entities
    {
        class User;
        class DiscussionTag;
        class DiscussionCategory;

        /**
        * Stores a discussion thread that contains messages
        * Repositories are responsible for updating the relationships between this message and other entities
        */
        class DiscussionThread final : public Authorization::DiscussionThreadPrivilegeStore,
                                       public StoresEntityPointer<DiscussionThread>,
                                       private boost::noncopyable
        {
        public:
            const auto& id()                        const { return id_; }

                   auto created()                   const { return created_; }
            const auto& creationDetails()           const { return creationDetails_; }

            const auto& createdBy()                 const { return createdBy_; }

            const auto& name()                      const { return name_; }

            const auto& messages()                  const { return messages_; }
                   auto messageCount()              const { return messages_.count(); }

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
                return lastUpdated_ ? lastUpdated_->by.toConst() : EntityPointer<const User>{};
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
                PinDisplayOrder
            };

            typedef Helpers::JsonReadyStringWithSortKey<1> NameType;

            struct ChangeNotification final
            {
                std::function<void(const DiscussionThread&)> onPrepareUpdateName;
                std::function<void(const DiscussionThread&)> onUpdateName;

                std::function<void(const DiscussionThread&)> onPrepareUpdateLastUpdated;
                std::function<void(const DiscussionThread&)> onUpdateLastUpdated;

                std::function<void(const DiscussionThread&)> onPrepareUpdateLatestMessageCreated;
                std::function<void(const DiscussionThread&)> onUpdateLatestMessageCreated;

                std::function<void(const DiscussionThread&)> onPrepareUpdateMessageCount;
                std::function<void(const DiscussionThread&)> onUpdateMessageCount;

                std::function<void(const DiscussionThread&)> onPrepareUpdatePinDisplayOrder;
                std::function<void(const DiscussionThread&)> onUpdatePinDisplayOrder;
            };

            static auto& changeNotifications() { return changeNotifications_; }

            DiscussionThread(IdType id, User& createdBy, NameType&& name, Timestamp created, VisitDetails creationDetails)
                : id_(std::move(id)), created_(created), creationDetails_(std::move(creationDetails)),
                  createdBy_(createdBy), name_(std::move(name))
            {
                messages_.onPrepareCountChange() = [this]() { changeNotifications_.onPrepareUpdateMessageCount(*this); };
                messages_.onCountChange()        = [this]() { changeNotifications_.onUpdateMessageCount(*this); };
            }

            void updateName(NameType&& name)
            {
                changeNotifications_.onPrepareUpdateName(*this);
                name_ = std::move(name);
                changeNotifications_.onUpdateName(*this);
            }

            void updateLastUpdated(Timestamp value)
            {
                if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
                if (lastUpdated_->at == value) return;

                changeNotifications_.onPrepareUpdateLastUpdated(*this);
                lastUpdated_->at = value;
                changeNotifications_.onUpdateLastUpdated(*this);
            }

            void updateLastUpdatedDetails(VisitDetails&& details)
            {
                if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
                lastUpdated_->details = std::move(details);
            }

            void updateLastUpdatedReason(std::string&& reason)
            {
                if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
                lastUpdated_->reason = std::move(reason);
            }

            void updateLastUpdatedBy(EntityPointer<User> by)
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
            bool& aboutToBeDeleted() { return aboutToBeDeleted_; }

            void updatePinDisplayOrder(uint16_t value)
            {
                changeNotifications_.onPrepareUpdatePinDisplayOrder(*this);
                pinDisplayOrder_ = value;
                changeNotifications_.onUpdatePinDisplayOrder(*this);
            }

            void updateLatestMessageCreated(Timestamp value)
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

            bool addTag(EntityPointer<DiscussionTag> tag);
            bool removeTag(EntityPointer<DiscussionTag> tag);

            bool addCategory(EntityPointer<DiscussionCategory> category);
            bool removeCategory(EntityPointer<DiscussionCategory> category);

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

            //store the timestamp of the latest visibile change in order to be able to
            //detect when to return a status that nothing has changed since a provided timestamp
            //Note: do not use as index in collection, the indexes would not always be updated
            Timestamp latestVisibleChange_{0};

            //store the timestamp of the latest message in the collection that was created
            //as it's expensive to retrieve it every time
            Timestamp latestMessageCreated_{0};

            uint16_t pinDisplayOrder_{0};
            mutable std::atomic_int_fast64_t visited_{0};
            bool aboutToBeDeleted_ = false;

            std::set<boost::uuids::uuid> visitorsSinceLastEdit_;

            std::set<EntityPointer<DiscussionTag>> tags_;
            std::set<EntityPointer<DiscussionCategory>> categories_;
            std::unordered_map<IdType, EntityPointer<User>> subscribedUsers_;
        };

        typedef EntityPointer<DiscussionThread> DiscussionThreadPtr;
        typedef EntityPointer<const DiscussionThread> DiscussionThreadConstPtr;
    }
}
