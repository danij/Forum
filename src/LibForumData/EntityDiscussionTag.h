#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollection.h"

#include <string>
#include <memory>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Entities
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
                                    private boost::noncopyable
        {
        public:
            const auto& id()                 const { return id_; }

                   auto created()            const { return created_; }
            const auto& creationDetails()    const { return creationDetails_; }

             StringView uiBlob()             const { return uiBlob_; }
            const auto& name()               const { return name_; }

            const auto& threads()            const { return threads_; }

                   auto lastUpdated()        const { return lastUpdated_; }
            const auto& lastUpdatedDetails() const { return lastUpdatedDetails_; }
             StringView lastUpdatedReason()  const { return lastUpdatedReason_; }
            const auto& lastUpdatedBy()      const { return lastUpdatedBy_; }
            
                   auto threadCount()        const { return threads_.count(); }
                   auto messageCount()       const { return messageCount_; }

                   auto categories()         const { return Helpers::toConst(categories_); }

            Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(
                    Authorization::DiscussionThreadMessagePrivilege privilege) const override;
            Authorization::PrivilegeValueType getDiscussionThreadPrivilege(
                    Authorization::DiscussionThreadPrivilege privilege) const override;
            Authorization::PrivilegeDefaultDurationType getDiscussionThreadMessageDefaultPrivilegeDuration(
                    Authorization::DiscussionThreadMessageDefaultPrivilegeDuration privilege) const override;
            Authorization::PrivilegeValueType getDiscussionTagPrivilege(
                    Authorization::DiscussionTagPrivilege privilege) const override;

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                UIBlob
            };
            
            struct ChangeNotification
            {
                std::function<void(const DiscussionTag&)> onUpdateName;
                std::function<void(const DiscussionTag&)> onUpdateThreadCount;
                std::function<void(const DiscussionTag&)> onUpdateMessageCount;
            };
            
            static auto& changeNotifications() { return changeNotifications_; }

            DiscussionTag(IdType id, Timestamp created, VisitDetails creationDetails, 
                          Authorization::ForumWidePrivilegeStore& forumWidePrivileges)
                : id_(std::move(id)), created_(created), creationDetails_(std::move(creationDetails)),
                  forumWidePrivileges_(forumWidePrivileges)
            {}

            void updateName(Helpers::StringWithSortKey&& name)
            {
                name_ = std::move(name);
                changeNotifications_.onUpdateName(*this);
            }
            std::string& uiBlob() { return uiBlob_; }

            auto& lastUpdated() { return lastUpdated_; }
            auto& lastUpdatedDetails() { return lastUpdatedDetails_; }
            auto& lastUpdatedReason() { return lastUpdatedReason_; }

            auto& messageCount() { return messageCount_; }
            void updateMessageCount(int_fast32_t value)
            {
                messageCount_ = value;
                changeNotifications_.onUpdateMessageCount(*this);
            }

            bool insertDiscussionThread(DiscussionThreadPtr thread);
            bool deleteDiscussionThread(DiscussionThreadPtr thread);

            bool addCategory(EntityPointer<DiscussionCategory> category);
            bool removeCategory(EntityPointer<DiscussionCategory> category);

        private:
            static ChangeNotification changeNotifications_;

            IdType id_;
            Timestamp created_ = 0;
            VisitDetails creationDetails_;

            Helpers::StringWithSortKey name_;
            std::string uiBlob_;

            DiscussionThreadCollectionWithHashedId threads_;

            Timestamp lastUpdated_ = 0;
            VisitDetails lastUpdatedDetails_;
            std::string lastUpdatedReason_;
            boost::optional<EntityPointer<User>> lastUpdatedBy_;
            
            int_fast32_t messageCount_ = 0;
            std::set<EntityPointer<DiscussionCategory>> categories_;

            Authorization::ForumWidePrivilegeStore& forumWidePrivileges_;
        };

        typedef EntityPointer<DiscussionTag> DiscussionTagPtr;
    }
}
