#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollection.h"
#include "EntityDiscussionTag.h"
#include "EntityUser.h"

#include <string>
#include <set>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Entities
    {
        /**
        * Stores hierarchical category that groups discussion threads
        * Repositories are responsible for updating the relationships between this message and other entities
        *
        * The discussion category manages the message count and the total thread/message counts
        * when adding/removing threads and/or tags
        */
        class DiscussionCategory final : public Authorization::DiscussionCategoryPrivilegeStore,
                                         public StoresEntityPointer<DiscussionCategory>,
                                         private boost::noncopyable
        {
        public:
            const auto& id()                 const { return id_; }

                   auto created()            const { return created_; }
            const auto& creationDetails()    const { return creationDetails_; }

            const auto& name()               const { return name_; }

             StringView description()        const { return description_; }
                   auto parent()             const { return parent_.toConst(); }

                   auto displayOrder()       const { return displayOrder_; }
                   bool isRootCategory()     const { return ! parent_; }

                   auto lastUpdated()        const { return lastUpdated_; }
            const auto& lastUpdatedDetails() const { return lastUpdatedDetails_; }
             StringView lastUpdatedReason()  const { return lastUpdatedReason_; }
                   auto lastUpdatedBy()      const { return lastUpdatedBy_.toConst(); }
                   
            const auto& threads()            const { return threads_; }
                   auto threadCount()        const { return threads_.count(); }

                   auto messageCount()       const { return messageCount_; }
                   
                   auto threadTotalCount()   const { return totalThreads_.count(); }
                   auto messageTotalCount()  const { return totalThreads_.messageCount(); }
                   
                   /**
                   * Returns the latest message based on all thread references held
                   */
                   auto latestMessage()      const { return totalThreads_.latestMessage(); }

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

            struct ChangeNotification
            {
                std::function<void(const DiscussionCategory&)> onPrepareUpdateName;
                std::function<void(const DiscussionCategory&)> onUpdateName;

                std::function<void(const DiscussionCategory&)> onPrepareUpdateMessageCount;
                std::function<void(const DiscussionCategory&)> onUpdateMessageCount;

                std::function<void(const DiscussionCategory&)> onPrepareUpdateDisplayOrder;
                std::function<void(const DiscussionCategory&)> onUpdateDisplayOrder;
            };

            static auto& changeNotifications() { return changeNotifications_; }
            
            DiscussionCategory(IdType id, NameType&& name, Timestamp created, VisitDetails creationDetails,
                               Authorization::ForumWidePrivilegeStore& forumWidePrivileges)
                : id_(std::move(id)), created_(created), creationDetails_(std::move(creationDetails)),
                  name_(std::move(name)), forumWidePrivileges_(forumWidePrivileges)
            {}

            void updateName(NameType&& name)
            {
                changeNotifications_.onPrepareUpdateName(*this);
                name_ = std::move(name);
                changeNotifications_.onUpdateName(*this);
            }

            void updateDisplayOrder(int_fast16_t value)
            {
                auto newValue = std::max(static_cast<int_fast16_t>(0), value);
                if (displayOrder_ == value) return;

                changeNotifications_.onPrepareUpdateDisplayOrder(*this);
                displayOrder_ = newValue;
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
            auto& lastUpdatedReason()  { return lastUpdatedReason_; }
            auto& lastUpdatedBy()      { return lastUpdatedBy_; }
            
            auto& threads()            { return threads_; }
            auto& tags()               { return tags_; }
            auto& children()           { return children_; }

            bool addChild(EntityPointer<DiscussionCategory> category);
            bool removeChild(EntityPointer<DiscussionCategory> category);
            bool hasAncestor(EntityPointer<DiscussionCategory> ancestor);

            bool insertDiscussionThread(DiscussionThreadPtr thread);
            bool deleteDiscussionThread(DiscussionThreadPtr thread);
            void deleteDiscussionThreadIfNoOtherTagsReferenceIt(DiscussionThreadPtr thread);

            bool addTag(DiscussionTagPtr tag);
            bool removeTag(DiscussionTagPtr tag);
            bool containsTag(DiscussionTagPtr tag) const;

            void updateMessageCount(DiscussionThreadPtr thread, int_fast32_t delta);

            void removeTotalsFromChild(const DiscussionCategory& childCategory);
            void addTotalsFromChild(const DiscussionCategory& childCategory);

        private:
            static ChangeNotification changeNotifications_;

            IdType id_;
            Timestamp created_{0};
            VisitDetails creationDetails_;

            NameType name_;
            std::string description_;
            int_fast16_t displayOrder_{0};
            int_fast32_t messageCount_{0};
            EntityPointer<DiscussionCategory> parent_;

            Timestamp lastUpdated_{0};
            VisitDetails lastUpdatedDetails_;
            std::string lastUpdatedReason_;
            UserPtr lastUpdatedBy_;

            DiscussionThreadCollectionWithHashedIdAndPinOrder threads_;
            DiscussionThreadCollectionWithReferenceCountAndMessageCount totalThreads_;

            std::set<DiscussionTagPtr> tags_;
            //enable fast search of children, client can sort them on display order
            std::set<EntityPointer<DiscussionCategory>> children_;

            Authorization::ForumWidePrivilegeStore& forumWidePrivileges_;
        };

        typedef EntityPointer<DiscussionCategory> DiscussionCategoryPtr;
        typedef EntityPointer<const DiscussionCategory> DiscussionCategoryConstPtr;
    }
}
