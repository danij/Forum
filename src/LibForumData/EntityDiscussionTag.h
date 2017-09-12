#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollection.h"

#include <string>

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
                                    public StoresEntityPointer<DiscussionTag>,
                                    private boost::noncopyable
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
             StringView lastUpdatedReason()  const { return lastUpdatedReason_; }
                   auto lastUpdatedBy()      const { return lastUpdatedBy_.toConst(); }

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

            typedef Helpers::JsonReadyStringWithSortKey<64> NameType;

            struct ChangeNotification
            {
                std::function<void(const DiscussionTag&)> onPrepareUpdateName;
                std::function<void(const DiscussionTag&)> onUpdateName;

                std::function<void(const DiscussionTag&)> onPrepareUpdateThreadCount;
                std::function<void(const DiscussionTag&)> onUpdateThreadCount;

                std::function<void(const DiscussionTag&)> onPrepareUpdateMessageCount;
                std::function<void(const DiscussionTag&)> onUpdateMessageCount;
            };

            static auto& changeNotifications() { return changeNotifications_; }

            DiscussionTag(IdType id, NameType&& name, Timestamp created, VisitDetails creationDetails,
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
            std::string& uiBlob()      { return uiBlob_; }

            auto& lastUpdated()        { return lastUpdated_; }
            auto& lastUpdatedDetails() { return lastUpdatedDetails_; }
            auto& lastUpdatedReason()  { return lastUpdatedReason_; }
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
            bool deleteDiscussionThread(DiscussionThreadPtr thread);

            bool addCategory(EntityPointer<DiscussionCategory> category);
            bool removeCategory(EntityPointer<DiscussionCategory> category);

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
            std::string lastUpdatedReason_;
            EntityPointer<User> lastUpdatedBy_;

            int_fast32_t messageCount_{0};
            std::set<EntityPointer<DiscussionCategory>> categories_;

            Authorization::ForumWidePrivilegeStore& forumWidePrivileges_;
        };

        typedef EntityPointer<DiscussionTag> DiscussionTagPtr;
        typedef EntityPointer<const DiscussionTag> DiscussionTagConstPtr;
    }
}
