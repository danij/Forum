#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollectionBase.h"
#include "EntityDiscussionTag.h"
#include "EntityDiscussionThreadRefCountedCollection.h"

#include <string>
#include <memory>
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
        class DiscussionCategory final :  public DiscussionThreadCollectionBase<HashIndexForId>,
                                          public Authorization::DiscussionCategoryPrivilegeStore,
            private boost::noncopyable
        {
        public:
            const auto& id() const { return id_; }

            auto created() const { return created_; }

            const auto& creationDetails() const { return creationDetails_; }
            auto& creationDetails() { return creationDetails_; }

            auto lastUpdated() const { return lastUpdated_; }
            auto& lastUpdated() { return lastUpdated_; }

            const auto& lastUpdatedDetails() const { return lastUpdatedDetails_; }
            auto& lastUpdatedDetails() { return lastUpdatedDetails_; }

            StringView lastUpdatedReason() const { return lastUpdatedReason_; }
            auto& lastUpdatedReason() { return lastUpdatedReason_; }

            const auto& lastUpdatedBy() const { return lastUpdatedBy_; }

            const auto& name() const { return name_; }
            void updateName(Helpers::StringWithSortKey&& name)
            {
                name_ = std::move(name);
                changeNotifications_.onUpdateName(*this);
            }


            StringView    description()       const { return description_; }
            std::string&  description()             { return description_; }

            int_fast16_t  displayOrder()      const { return displayOrder_; }
            void updateDisplayOrder(int_fast16_t value)
            {
                displayOrder_ = std::max(static_cast<int_fast16_t>(0), value);
                changeNotifications_.onUpdateDisplayOrder(*this);
            }

            int_fast32_t  messageCount()      const { return messageCount_; }

            int_fast32_t  threadTotalCount()  const { return totalThreads_.threadsById().size(); }
            int_fast32_t  messageTotalCount() const { return totalThreads_.messageCount(); }

            auto&         parentWeak()              { return parent_; }

            auto          tags()              const { return Helpers::toConst(tags_); }
            auto&         tags()                    { return tags_; }

            auto          children()          const { return Helpers::toConst(children_); }
            auto&         children()                { return children_; }

            bool isRootCategory()             const { return parent_.expired(); }

            int_fast16_t displayOrderWithRootPriority() const
            {
                //use a negative value for the root elements so they are sorted before the others
                //preserve the sort order based on ascending display order
                return isRootCategory() ? std::numeric_limits<int_fast16_t>::min() + displayOrder_ : displayOrder_;
            }

            Authorization::PrivilegeValueType getDiscussionCategoryPrivilege(Authorization::DiscussionCategoryPrivilege privilege) const override;

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                Description,
                DisplayOrder,
                Parent
            };

            struct ChangeNotification
            {
                std::function<void(const DiscussionCategory&)> onUpdateName;
                std::function<void(const DiscussionCategory&)> onUpdateMessageCount;
                std::function<void(const DiscussionCategory&)> onUpdateDisplayOrder;
            };

            static auto& changeNotifications() { return changeNotifications_; }
            
            DiscussionCategory(IdType id, Timestamp created, Authorization::ForumWidePrivilegeStore& forumWidePrivileges)
                : id_(std::move(id)), created_(created),
            notifyChangeFn_(&DiscussionCategory::emptyNotifyChange), forumWidePrivileges_(forumWidePrivileges) { }

            bool addChild(std::shared_ptr<DiscussionCategory> category)
            {
                return std::get<1>(children_.insert(std::move(category)));
            }

            bool removeChild(const std::shared_ptr<DiscussionCategory>& category)
            {
                return children_.erase(category) > 0;
            }

            bool hasAncestor(const std::weak_ptr<DiscussionCategory>& ancestor)
            {
                if (Helpers::ownerEqual(parent_, ancestor)) return true;
                if (auto parentShared = parent_.lock())
                {
                    return parentShared->hasAncestor(ancestor);
                }
                return false;
            }

            template<typename TAction>
            void executeActionWithParentCategoryIfAvailable(TAction&& action) const
            {
                if (auto parentShared = parent_.lock())
                {
                    action(const_cast<const DiscussionCategory&>(*parentShared));
                }
            }

            template<typename TAction>
            void executeActionWithParentCategoryIfAvailable(TAction&& action)
            {
                if (auto parentShared = parent_.lock())
                {
                    action(*parentShared);
                }
            }

            bool insertDiscussionThread(const DiscussionThreadRef& thread) override;

            void modifyDiscussionThread(ThreadIdIteratorType iterator,
                                        std::function<void(DiscussionThread&)>&& modifyFunction) override;

            DiscussionThreadRef deleteDiscussionThread(ThreadIdIteratorType iterator) override;

            void deleteDiscussionThreadIfNoOtherTagsReferenceIt(const DiscussionThreadRef& thread);

            //strong links are more elaborate and propagate changes
            bool addTag(const DiscussionTagRef& tag);
            bool removeTag(const DiscussionTagRef& tag);

            bool containsTag(const DiscussionTagRef& tag) const
            {
                return tags_.find(tag) != tags_.end();
            }

            void updateMessageCount(const DiscussionThreadRef& thread, int_fast32_t delta);
            /**
             * Returns the latest message based on all thread references held
             */
            const DiscussionThreadMessage* latestMessage() const { return totalThreads_.latestMessage(); }

            void resetTotals();
            void recalculateTotals();

            typedef std::function<void(DiscussionCategory&)> NotifyChangeActionType;
            auto& notifyChange() { return notifyChangeFn_; }

        private:
            IdType id_;
            Timestamp created_ = 0;
            VisitDetails creationDetails_;

            Timestamp lastUpdated_ = 0;
            VisitDetails lastUpdatedDetails_;
            std::string lastUpdatedReason_;

            boost::optional<UserPtr> lastUpdatedBy_;

            Helpers::StringWithSortKey name_;
            std::string description_;
            int_fast16_t displayOrder_ = 0;
            int_fast32_t messageCount_ = 0;
            DiscussionThreadRefCountedCollection<HashIndexForId> totalThreads_;
            std::weak_ptr<DiscussionCategory> parent_;
            //enable fast search of children, client can sort them on display order
            std::set<std::shared_ptr<DiscussionCategory>, std::owner_less<std::shared_ptr<DiscussionCategory>>> children_;
            std::set<DiscussionTagRef, std::owner_less<DiscussionTagRef>> tags_;
            NotifyChangeActionType notifyChangeFn_;
            Authorization::ForumWidePrivilegeStore& forumWidePrivileges_;

            static void emptyNotifyChange(DiscussionCategory&) { }

            static ChangeNotification changeNotifications_;
        };

        typedef EntityPointer<DiscussionCategory> DiscussionCategoryPtr;
    }
}
