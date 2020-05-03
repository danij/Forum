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
#include "EntityDiscussionTag.h"
#include "EntityUser.h"
#include "TypeHelpers.h"

#include <string>

#include <boost/noncopyable.hpp>
#include <boost/container/flat_set.hpp>

namespace Forum::Entities
{
    /**
    * Stores hierarchical category that groups discussion threads
    * Repositories are responsible for updating the relationships between this message and other entities
    *
    * The discussion category manages the message count and the total thread/message counts
    * when adding/removing threads and/or tags
    */
    class DiscussionCategory final : public Authorization::DiscussionCategoryPrivilegeStore,
                                     boost::noncopyable
    {
    public:
        const auto& id()                 const { return id_; }

               auto created()            const { return created_; }
        const auto& creationDetails()    const { return creationDetails_; }

        const auto& name()               const { return name_; }

         StringView description()        const { return description_; }
               auto parent()             const { return Helpers::toConstPtr(parent_); }

               auto displayOrder()       const { return displayOrder_; }
               bool isRootCategory()     const { return ! parent_; }

               auto lastUpdated()        const { return lastUpdated_; }
        const auto& lastUpdatedDetails() const { return lastUpdatedDetails_; }
               auto lastUpdatedBy()      const { return static_cast<const User*>(lastUpdatedBy_); }

        const auto& threads()            const { return threads_; }
               auto threadCount()        const { return threads_.count(); }

               auto messageCount()       const { return messageCount_; }

               auto threadTotalCount()   const { return totalThreads_.count(); }
               auto messageTotalCount()  const { return totalThreads_.messageCount(); }

        const DiscussionThreadMessage* latestMessage() const;

               auto tags()               const { return Helpers::toConst(tags_); }
               auto children()           const { return Helpers::toConst(children_); }

        auto displayOrderWithRootPriority() const
        {
            //use a negative value for the root elements so they are sorted before the others
            //preserve the sort order based on ascending display order
            return isRootCategory() ? std::numeric_limits<int_fast16_t>::min() + displayOrder_ : displayOrder_;
        }

        Authorization::PrivilegeValueType getDiscussionCategoryPrivilege(
                Authorization::DiscussionCategoryPrivilege privilege) const override;

        enum ChangeType : uint32_t
        {
            None = 0,
            Name,
            Description,
            DisplayOrder,
            Parent
        };

        typedef Helpers::JsonReadyStringWithSortKey<256> NameType;

        struct ChangeNotification final
        {
            std::function<void(DiscussionCategory&)> onPrepareUpdateName;
            std::function<void(DiscussionCategory&)> onUpdateName;

            std::function<void(DiscussionCategory&)> onPrepareUpdateMessageCount;
            std::function<void(DiscussionCategory&)> onUpdateMessageCount;

            std::function<void(DiscussionCategory&)> onPrepareUpdateDisplayOrder;
            std::function<void(DiscussionCategory&)> onUpdateDisplayOrder;
        };

        static auto& changeNotifications() { return changeNotifications_; }

        DiscussionCategory(IdType id, NameType&& name, Timestamp created, VisitDetails creationDetails,
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

        void updateDisplayOrder(const int_fast16_t value)
        {
            const auto newValue = std::max(static_cast<int_fast16_t>(0), value);
            if (displayOrder_ == value) return;

            changeNotifications_.onPrepareUpdateDisplayOrder(*this);
            displayOrder_ = newValue;
            changeNotifications_.onUpdateDisplayOrder(*this);
        }

        void updateParent(DiscussionCategory* const newParent)
        {
            //displayOrderWithRootPriority depends on the parent
            changeNotifications_.onPrepareUpdateDisplayOrder(*this);
            parent_ = newParent;
            changeNotifications_.onUpdateDisplayOrder(*this);
        }

        void stopBatchInsert()
        {
            threads_.stopBatchInsert();
            totalThreads_.stopBatchInsert();
        }

        auto& parent()             { return parent_; }
        auto& description()        { return description_; }

        auto& lastUpdated()        { return lastUpdated_; }
        auto& lastUpdatedDetails() { return lastUpdatedDetails_; }
        auto& lastUpdatedBy()      { return lastUpdatedBy_; }

        auto& threads()            { return threads_; }
        auto& tags()               { return tags_; }
        auto& children()           { return children_; }

        bool addChild(DiscussionCategory* category);
        bool removeChild(DiscussionCategory* category);
        bool hasAncestor(DiscussionCategory* ancestor);

        bool insertDiscussionThread(DiscussionThreadPtr thread);
        bool insertDiscussionThreads(DiscussionThreadPtr* threads, size_t count);
        bool deleteDiscussionThread(DiscussionThreadPtr thread, bool deleteMessages, bool onlyThisCategory);
        void deleteDiscussionThreadIfNoOtherTagsReferenceIt(DiscussionThreadPtr thread, bool deleteMessages);

        bool addTag(DiscussionTagPtr tag);
        bool removeTag(DiscussionTagPtr tag);
        bool containsTag(DiscussionTagPtr tag) const;

        void updateMessageCount(DiscussionThreadPtr thread, int_fast32_t delta);

        void removeTotalsFromChild(const DiscussionCategory& childCategory);
        void addTotalsFromChild(const DiscussionCategory& childCategory);

    private:
        bool insertDiscussionThreadsOfTag(DiscussionTagPtr tag);

    private:
        static ChangeNotification changeNotifications_;

        IdType id_;
        Timestamp created_{0};
        VisitDetails creationDetails_;

        NameType name_;
        std::string description_;
        int_fast16_t displayOrder_{0};
        int_fast32_t messageCount_{0};
        DiscussionCategory* parent_{};

        Timestamp lastUpdated_{0};
        VisitDetails lastUpdatedDetails_;
        UserPtr lastUpdatedBy_{};

        DiscussionThreadCollectionWithHashedIdAndPinOrder threads_;
        DiscussionThreadCollectionWithReferenceCountAndMessageCount totalThreads_;

        boost::container::flat_set<DiscussionTagPtr> tags_;
        //enable fast search of children, client can sort them on display order
        boost::container::flat_set<DiscussionCategory*> children_;

        Authorization::ForumWidePrivilegeStore& forumWidePrivileges_;
    };

    typedef DiscussionCategory* DiscussionCategoryPtr;
    typedef const DiscussionCategory* DiscussionCategoryConstPtr;
}
