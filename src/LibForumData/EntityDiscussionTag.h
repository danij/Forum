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
#include "EntityDiscussionThreadCollection.h"
#include "TypeHelpers.h"

#include <string>

#include <boost/noncopyable.hpp>
#include <boost/container/flat_set.hpp>

namespace Forum::Entities
{
    class User;
    class DiscussionCategory;

    /**
    * Stores a discussion tag that groups threads of similar discussions
    * Repositories are responsible for updating the relationships between this message and other entities
    *
    * The tag manages the message count and also notifies any discussion categories when a thread is added or removed
    */
    class DiscussionTag final : public Authorization::DiscussionTagPrivilegeStore,
                                boost::noncopyable
    {
    public:
        const auto& id()                 const { return id_; }

               auto created()            const { return created_; }
        const auto& creationDetails()    const { return creationDetails_; }

        const auto& name()               const { return name_; }
         StringView uiBlob()             const { return uiBlob_; }

        const auto& threads()            const { return threads_; }

               auto lastUpdated()        const { return lastUpdated_; }
        const auto& lastUpdatedDetails() const { return lastUpdatedDetails_; }
               auto lastUpdatedBy()      const { return Helpers::toConstPtr(lastUpdatedBy_); }

               auto threadCount()        const { return threads_.count(); }

               auto messageCount()       const { return messageCount_; }

               auto categories()         const { return Helpers::toConst(categories_); }

        Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(
                Authorization::DiscussionThreadMessagePrivilege privilege) const override;
        Authorization::PrivilegeValueType getDiscussionThreadPrivilege(
                Authorization::DiscussionThreadPrivilege privilege) const override;
        Authorization::PrivilegeValueType getDiscussionTagPrivilege(
                Authorization::DiscussionTagPrivilege privilege) const override;

        enum ChangeType : uint32_t
        {
            None = 0,
            Name,
            UIBlob
        };

        typedef Helpers::JsonReadyStringWithSortKey<64> NameType;

        struct ChangeNotification final
        {
            std::function<void(DiscussionTag&)> onPrepareUpdateName;
            std::function<void(DiscussionTag&)> onUpdateName;

            std::function<void(DiscussionTag&)> onPrepareUpdateThreadCount;
            std::function<void(DiscussionTag&)> onUpdateThreadCount;

            std::function<void(DiscussionTag&)> onPrepareUpdateMessageCount;
            std::function<void(DiscussionTag&)> onUpdateMessageCount;
        };

        static auto& changeNotifications() { return changeNotifications_; }

        DiscussionTag(IdType id, NameType&& name, Timestamp created, VisitDetails creationDetails,
                      Authorization::ForumWidePrivilegeStore& forumWidePrivileges)
            : id_(id), created_(created), creationDetails_(creationDetails),
              name_(std::move(name)), forumWidePrivileges_(forumWidePrivileges)
        {}

        void updateName(NameType&& name)
        {
            changeNotifications_.onPrepareUpdateName(*this);
            name_ = std::move(name);
            changeNotifications_.onUpdateName(*this);
        }
        std::string& uiBlob()      { return uiBlob_; }

        auto& lastUpdated()        { return lastUpdated_; }
        auto& lastUpdatedDetails() { return lastUpdatedDetails_; }
        auto& lastUpdatedBy()      { return lastUpdatedBy_; }

        auto& threads()            { return threads_; }
        auto& categories()         { return categories_; }

        void updateMessageCount(int_fast32_t delta)
        {
            if (0 == delta) return;

            changeNotifications_.onPrepareUpdateMessageCount(*this);
            messageCount_ += delta;
            changeNotifications_.onUpdateMessageCount(*this);
        }

        bool insertDiscussionThread(DiscussionThreadPtr thread);
        bool insertDiscussionThreads(DiscussionThreadPtr* threads, size_t count);
        bool deleteDiscussionThread(DiscussionThreadPtr thread, bool deleteMessages);

        bool addCategory(DiscussionCategory* category);
        bool removeCategory(DiscussionCategory* category);

    private:
        static ChangeNotification changeNotifications_;

        IdType id_;
        Timestamp created_{0};
        VisitDetails creationDetails_;

        NameType name_;
        std::string uiBlob_;

        DiscussionThreadCollectionWithHashedId threads_;

        Timestamp lastUpdated_{0};
        VisitDetails lastUpdatedDetails_;
        User* lastUpdatedBy_{};

        int_fast32_t messageCount_{0};
        boost::container::flat_set<DiscussionCategory*> categories_;

        Authorization::ForumWidePrivilegeStore& forumWidePrivileges_;
    };

    typedef DiscussionTag* DiscussionTagPtr;
    typedef const DiscussionTag* DiscussionTagConstPtr;
}
