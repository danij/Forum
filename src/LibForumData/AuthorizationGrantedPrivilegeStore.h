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
#include "Entities.h"

#include <cstddef>
#include <tuple>

#include <boost/noncopyable.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Forum::Authorization
{
    struct DiscussionThreadMessagePrivilegeCheck final
    {
        DiscussionThreadMessagePrivilegeCheck() = default;

        DiscussionThreadMessagePrivilegeCheck(Entities::IdTypeRef userId,
                                              const Entities::DiscussionThreadMessage& message)
            : userId(userId), message(&message) {}
        ~DiscussionThreadMessagePrivilegeCheck() = default;
        

        DiscussionThreadMessagePrivilegeCheck(const DiscussionThreadMessagePrivilegeCheck&) = default;
        DiscussionThreadMessagePrivilegeCheck(DiscussionThreadMessagePrivilegeCheck&&) = default;

        DiscussionThreadMessagePrivilegeCheck& operator=(const DiscussionThreadMessagePrivilegeCheck&) = default;
        DiscussionThreadMessagePrivilegeCheck& operator=(DiscussionThreadMessagePrivilegeCheck&&) = default;

        Entities::IdType userId;
        const Entities::DiscussionThreadMessage* message = nullptr;
        bool allowedToShowMessage = false;
        bool allowedToShowUser = false;
        bool allowedToShowVotes = false;
        bool allowedToShowIpAddress = false;
        bool allowedToViewComments = false;
    };

    class GrantedPrivilegeStore final : boost::noncopyable
    {
    public:
        GrantedPrivilegeStore();

        void grantDiscussionThreadMessagePrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                                   PrivilegeValueIntType value, Entities::Timestamp now,
                                                   Entities::Timestamp expiresAt);

        void grantDiscussionThreadPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                            PrivilegeValueIntType value, Entities::Timestamp now,
                                            Entities::Timestamp expiresAt);

        void grantDiscussionTagPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                         PrivilegeValueIntType value, Entities::Timestamp now,
                                         Entities::Timestamp expiresAt);

        void grantDiscussionCategoryPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                              PrivilegeValueIntType value, Entities::Timestamp now,
                                              Entities::Timestamp expiresAt);

        void grantForumWidePrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                     PrivilegeValueIntType value, Entities::Timestamp now,
                                     Entities::Timestamp expiresAt);

        //isAllowed returns the privilege level with which access was granted or empty if not allowed

        PrivilegeValueType isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionThreadMessage& message,
                                     DiscussionThreadMessagePrivilege privilege, Entities::Timestamp now) const;
        PrivilegeValueType isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionThread& thread,
                                     DiscussionThreadMessagePrivilege privilege, Entities::Timestamp now) const;
        PrivilegeValueType isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionTag& tag,
                                     DiscussionThreadMessagePrivilege privilege, Entities::Timestamp now) const;

        PrivilegeValueType isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionThread& thread,
                                     DiscussionThreadPrivilege privilege, Entities::Timestamp now) const;
        PrivilegeValueType isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionTag& tag,
                                     DiscussionThreadPrivilege privilege, Entities::Timestamp now) const;

        PrivilegeValueType isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionTag& tag,
                                     DiscussionTagPrivilege privilege, Entities::Timestamp now) const;

        PrivilegeValueType isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionCategory& category,
                                     DiscussionCategoryPrivilege privilege, Entities::Timestamp now) const;

        PrivilegeValueType isAllowed(Entities::IdTypeRef userId,
                                     const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                     DiscussionThreadMessagePrivilege privilege, Entities::Timestamp now) const;
        PrivilegeValueType isAllowed(Entities::IdTypeRef userId,
                                     const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                     DiscussionThreadPrivilege privilege, Entities::Timestamp now) const;
        PrivilegeValueType isAllowed(Entities::IdTypeRef userId,
                                     const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                     DiscussionTagPrivilege privilege, Entities::Timestamp now) const;
        PrivilegeValueType isAllowed(Entities::IdTypeRef userId,
                                     const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                     DiscussionCategoryPrivilege privilege, Entities::Timestamp now) const;
        PrivilegeValueType isAllowed(Entities::IdTypeRef userId,
                                     const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                     ForumWidePrivilege privilege, Entities::Timestamp now) const;

        void computeDiscussionThreadMessageVisibilityAllowed(DiscussionThreadMessagePrivilegeCheck* items,
                                                             size_t nrOfItems, Entities::Timestamp now) const;

        typedef std::function<void(Entities::IdTypeRef, PrivilegeValueIntType,
                                   Entities::Timestamp, Entities::Timestamp)> EnumerationCallback;

        void enumerateDiscussionThreadMessagePrivileges(Entities::IdTypeRef id, EnumerationCallback&& callback) const;
        void enumerateDiscussionThreadPrivileges(Entities::IdTypeRef id, EnumerationCallback&& callback) const;
        void enumerateDiscussionTagPrivileges(Entities::IdTypeRef id, EnumerationCallback&& callback) const;
        void enumerateDiscussionCategoryPrivileges(Entities::IdTypeRef id, EnumerationCallback&& callback) const;
        void enumerateForumWidePrivileges(Entities::IdTypeRef id, EnumerationCallback&& callback) const;

        void enumerateDiscussionThreadMessagePrivilegesAssignedToUser(Entities::IdTypeRef userId,
                                                                      EnumerationCallback&& callback) const;
        void enumerateDiscussionThreadPrivilegesAssignedToUser(Entities::IdTypeRef userId,
                                                               EnumerationCallback&& callback) const;
        void enumerateDiscussionTagPrivilegesAssignedToUser(Entities::IdTypeRef userId,
                                                            EnumerationCallback&& callback) const;
        void enumerateDiscussionCategoryPrivilegesAssignedToUser(Entities::IdTypeRef userId,
                                                                 EnumerationCallback&& callback) const;
        void enumerateForumWidePrivilegesAssignedToUser(Entities::IdTypeRef userId,
                                                        EnumerationCallback&& callback) const;

        void calculateDiscussionThreadMessagePrivilege(Entities::IdTypeRef userId,
                                                       const Entities::DiscussionThreadMessage& message,
                                                       Entities::Timestamp now, PrivilegeValueType& positiveValue,
                                                       PrivilegeValueType& negativeValue) const;

        void calculateDiscussionThreadPrivilege(Entities::IdTypeRef userId, const Entities::DiscussionThread& thread,
                                                Entities::Timestamp now, PrivilegeValueType& positiveValue,
                                                PrivilegeValueType& negativeValue) const;

        void calculateDiscussionTagPrivilege(Entities::IdTypeRef userId, const Entities::DiscussionTag& tag,
                                             Entities::Timestamp now, PrivilegeValueType& positiveValue,
                                             PrivilegeValueType& negativeValue) const ;

        void calculateDiscussionCategoryPrivilege(Entities::IdTypeRef userId,
                                                  const Entities::DiscussionCategory& category,
                                                  Entities::Timestamp now, PrivilegeValueType& positiveValue,
                                                  PrivilegeValueType& negativeValue) const;

        void calculateForumWidePrivilege(Entities::IdTypeRef userId, Entities::Timestamp now,
                                         PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;
    private:

        void calculateDiscussionThreadMessagePrivilege(Entities::IdTypeRef userId,
                                                       const Entities::DiscussionThread& thread,
                                                       Entities::Timestamp now,
                                                       PrivilegeValueType& positiveValue,
                                                       PrivilegeValueType& negativeValue) const;


        typedef std::tuple<Entities::IdType, Entities::IdType> IdTuple;

        struct PrivilegeEntry
        {
            PrivilegeEntry(Entities::IdTypeRef userId, Entities::IdTypeRef entityId, PrivilegeValueIntType value,
                           Entities::Timestamp grantedAt, Entities::Timestamp expiresAt)
                : userAndEntity_({ userId, entityId }),
                  privilegeValue_(value), grantedAt_(grantedAt), expiresAt_(expiresAt)
            {
            }

            const IdTuple& userAndEntity() const { return userAndEntity_; }

            Entities::IdTypeRef   userId() const { return std::get<0>(userAndEntity_); }
            Entities::IdTypeRef entityId() const { return std::get<1>(userAndEntity_); }
            auto          privilegeValue() const { return privilegeValue_; }
            auto               grantedAt() const { return grantedAt_; }
            auto               expiresAt() const { return expiresAt_; }

        private:
            IdTuple userAndEntity_;
            PrivilegeValueIntType privilegeValue_;
            Entities::Timestamp grantedAt_;
            Entities::Timestamp expiresAt_;
        };

        struct PrivilegeEntryCollectionByUserIdEntityId {};
        struct PrivilegeEntryCollectionByUserId {};
        struct PrivilegeEntryCollectionByEntityId {};

        struct PrivilegeEntryCollectionIndices : boost::multi_index::indexed_by<

            boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByUserIdEntityId>,
                    const boost::multi_index::const_mem_fun<PrivilegeEntry, const IdTuple&,
                            &PrivilegeEntry::userAndEntity>>,

            boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByUserId>,
                    const boost::multi_index::const_mem_fun<PrivilegeEntry, Entities::IdTypeRef, &PrivilegeEntry::userId>>,

            boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByEntityId>,
                    const boost::multi_index::const_mem_fun<PrivilegeEntry, Entities::IdTypeRef, &PrivilegeEntry::entityId>>
        > {};

        typedef boost::multi_index_container<PrivilegeEntry, PrivilegeEntryCollectionIndices>
                PrivilegeEntryCollection;

        void calculatePrivilege(const PrivilegeEntryCollection& collection, Entities::IdTypeRef userId,
                                Entities::IdTypeRef entityId, Entities::Timestamp now,
                                PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

        PrivilegeEntryCollection discussionThreadMessageSpecificPrivileges_;
        PrivilegeEntryCollection discussionThreadSpecificPrivileges_;
        PrivilegeEntryCollection discussionTagSpecificPrivileges_;
        PrivilegeEntryCollection discussionCategorySpecificPrivileges_;
        PrivilegeEntryCollection forumWideSpecificPrivileges_;
        PrivilegeValueIntType defaultPrivilegeValueForLoggedInUser_;
    };

    struct SerializationRestriction final : private boost::noncopyable
    {
        SerializationRestriction(const GrantedPrivilegeStore& privilegeStore, 
                                 const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                 Entities::IdTypeRef userId, Entities::Timestamp now)
            : privilegeStore_(privilegeStore), forumWidePrivilegeStore_(forumWidePrivilegeStore), 
              userId_(userId), now_(now)
        {
        }

        const GrantedPrivilegeStore& privilegeStore() const { return privilegeStore_; }
              Entities::IdTypeRef            userId() const { return userId_; }
              Entities::Timestamp               now() const { return now_; }

        bool isAllowed(const Entities::DiscussionThreadMessage& message,
                       const DiscussionThreadMessagePrivilege privilege = DiscussionThreadMessagePrivilege::VIEW) const
        {
            return static_cast<bool>(privilegeStore_.isAllowed(userId_, message, privilege, now_));
        }

        bool isAllowed(const Entities::DiscussionThread& thread,
                       const DiscussionThreadPrivilege privilege = DiscussionThreadPrivilege::VIEW) const
        {
            return static_cast<bool>(privilegeStore_.isAllowed(userId_, thread, privilege, now_));
        }

        bool isAllowed(const Entities::DiscussionTag& tag,
                       const DiscussionTagPrivilege privilege = DiscussionTagPrivilege::VIEW) const
        {
            return static_cast<bool>(privilegeStore_.isAllowed(userId_, tag, privilege, now_));
        }

        bool isAllowed(const Entities::DiscussionCategory& category,
                       const DiscussionCategoryPrivilege privilege = DiscussionCategoryPrivilege::VIEW) const
        {
            return static_cast<bool>(privilegeStore_.isAllowed(userId_, category, privilege, now_));
        }

        bool isAllowed(const ForumWidePrivilegeStore& forumWidePrivilegeStore, 
                       const ForumWidePrivilege privilege = ForumWidePrivilege::ADD_USER) const
        {
            return static_cast<bool>(privilegeStore_.isAllowed(userId_, forumWidePrivilegeStore, privilege, now_));
        }

        bool isAllowed(const ForumWidePrivilege privilege = ForumWidePrivilege::ADD_USER) const
        {
            return isAllowed(forumWidePrivilegeStore_, privilege);
        }

        bool isAllowedToViewAnyAttachment() const
        {
            return isAllowed(ForumWidePrivilege::GET_ALL_ATTACHMENTS)
                || isAllowed(ForumWidePrivilege::GET_ATTACHMENTS_OF_USER);
        }

        bool isAllowedToViewAttachment(const Entities::Attachment& attachment, 
                                       const Entities::DiscussionThreadMessage& message) const
        {
            return (message.createdBy().id() == userId())
                || (attachment.createdBy().id() == userId())
                || attachment.approved()
                || isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_UNAPPROVED_ATTACHMENT);
        }

        bool isAllowedToViewMessageAttachments(const Entities::DiscussionThreadMessage& message) const
        {
            return isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_ATTACHMENT);
        }

        bool checkMessageAllowViewApproval(const Entities::DiscussionThreadMessage& message) const
        {
            if (message.approved())
            {
                return true;
            }
            return (message.createdBy().id() == userId())
                || isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_UNAPPROVED);
        }

        bool isAllowedToViewMessage(const Entities::DiscussionThreadMessage& message) const
        {
            return isAllowed(message, DiscussionThreadMessagePrivilege::VIEW)
                && checkMessageAllowViewApproval(message)
                && isAllowedToViewThread(*message.parentThread());
        }

        bool checkThreadAllowViewApproval(const Entities::DiscussionThread& thread) const
        {
            if (thread.approved())
            {
                return true;
            }
            return (thread.createdBy().id() == userId())
                || isAllowed(thread, DiscussionThreadPrivilege::VIEW_UNAPPROVED);
        }

        bool isAllowedToViewThread(const Entities::DiscussionThread& thread) const
        {
            return isAllowed(thread, DiscussionThreadPrivilege::VIEW)
                && checkThreadAllowViewApproval(thread);
        }

    private:
        const GrantedPrivilegeStore& privilegeStore_;
        const ForumWidePrivilegeStore& forumWidePrivilegeStore_;
        Entities::IdTypeRef userId_;
        Entities::Timestamp now_;
    };
}
