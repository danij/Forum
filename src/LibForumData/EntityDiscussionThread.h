#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadMessageCollection.h"
#include "StringHelpers.h"

#include <atomic>
#include <string>
#include <set>

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

                   auto lastUpdated()               const { return lastUpdated_; }
            const auto& lastUpdatedDetails()        const { return lastUpdatedDetails_; }
             StringView lastUpdatedReason()         const { return lastUpdatedReason_; }

                   auto lastUpdatedBy()             const { return lastUpdatedBy_.toConst(); }

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
            Authorization::PrivilegeDefaultDurationType getDiscussionThreadMessageDefaultPrivilegeDuration(
                    Authorization::DiscussionThreadMessageDefaultPrivilegeDuration privilege) const override;

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                PinDisplayOrder
            };

            typedef Helpers::JsonReadyStringWithSortKey<1> NameType;

            struct ChangeNotification
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
                if (lastUpdated_ == value) return;

                changeNotifications_.onPrepareUpdateLastUpdated(*this);
                lastUpdated_ = value;
                changeNotifications_.onUpdateLastUpdated(*this);
            }

            auto& lastUpdatedDetails()  { return lastUpdatedDetails_; }
            auto& lastUpdatedReason()   { return lastUpdatedReason_; }
            auto& latestVisibleChange() { return latestVisibleChange_; }
            auto& lastUpdatedBy()       { return lastUpdatedBy_; }

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

            void insertMessage(DiscussionThreadMessagePtr message);
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
            DiscussionThreadMessageCollection messages_;

            Timestamp lastUpdated_{0};
            VisitDetails lastUpdatedDetails_;
            std::string lastUpdatedReason_;
            EntityPointer<User> lastUpdatedBy_;

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
            std::set<EntityPointer<User>> subscribedUsers_;
        };

        typedef EntityPointer<DiscussionThread> DiscussionThreadPtr;
        typedef EntityPointer<const DiscussionThread> DiscussionThreadConstPtr;
    }
}
