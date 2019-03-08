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
#include "AuthorizationGrantedPrivilegeStore.h"

#include "EntityUserCollection.h"
#include "EntityDiscussionThreadCollection.h"
#include "EntityDiscussionThreadMessageCollection.h"
#include "EntityDiscussionTagCollection.h"
#include "EntityDiscussionCategoryCollection.h"
#include "EntityMessageCommentCollection.h"
#include "EntityPrivateMessageCollection.h"
#include "EntityAttachmentCollection.h"

#include <memory>

#include <boost/noncopyable.hpp>

namespace Forum::Entities
{
    /**
     * Stores references to all entities present in memory
     * Upon deleting an entity, the collection also removes the entity from any other collection if might have been part of
     */
    class EntityCollection final : public Authorization::ForumWidePrivilegeStore,
                                   boost::noncopyable
    {
    public:
        explicit EntityCollection(StringView messagesFile);
        ~EntityCollection();

        const Authorization::GrantedPrivilegeStore& grantedPrivileges() const;
              Authorization::GrantedPrivilegeStore& grantedPrivileges();

        StringView getMessageContentPointer(size_t offset, size_t size);

        UserPtr                    createUser(IdType id, User::NameType&& name, Timestamp created,
                                              VisitDetails creationDetails);
        DiscussionThreadPtr        createDiscussionThread(IdType id, User& createdBy, DiscussionThread::NameType&& name,
                                                          Timestamp created, VisitDetails creationDetails, bool approved);
        DiscussionThreadMessagePtr createDiscussionThreadMessage(IdType id, User& createdBy, Timestamp created,
                                                                 VisitDetails creationDetails, bool approved);
        DiscussionTagPtr           createDiscussionTag(IdType id, DiscussionTag::NameType&& name, Timestamp created,
                                                       VisitDetails creationDetails);
        DiscussionCategoryPtr      createDiscussionCategory(IdType id, DiscussionCategory::NameType&& name,
                                                            Timestamp created, VisitDetails creationDetails);
        MessageCommentPtr          createMessageComment(IdType id, DiscussionThreadMessage& message, User& createdBy,
                                                        Timestamp created, VisitDetails creationDetails);
        PrivateMessagePtr          createPrivateMessage(IdType id, User& source, User& destination, Timestamp created,
                                                        VisitDetails creationDetails, PrivateMessage::ContentType&& content);
        AttachmentPtr              createAttachment(IdType id, Timestamp created, VisitDetails creationDetails,
                                                    User& createdBy, Attachment::NameType&& name, uint64_t size,
                                                    bool approved);

        const UserCollection&                         users() const;
              UserCollection&                         users();
        const DiscussionThreadCollectionWithHashedId& threads() const;
              DiscussionThreadCollectionWithHashedId& threads();
        const DiscussionThreadMessageCollection&      threadMessages() const;
              DiscussionThreadMessageCollection&      threadMessages();
        const DiscussionTagCollection&                tags() const;
              DiscussionTagCollection&                tags();
        const DiscussionCategoryCollection&           categories() const;
              DiscussionCategoryCollection&           categories();
        const MessageCommentCollection&               messageComments() const;
              MessageCommentCollection&               messageComments();
        const PrivateMessageGlobalCollection&         privateMessages() const;
              PrivateMessageGlobalCollection&         privateMessages();
        const AttachmentCollection&                   attachments() const;
              AttachmentCollection&                   attachments();

        void insertUser(UserPtr user);
        void deleteUser(UserPtr user);

        void insertDiscussionThread(DiscussionThreadPtr thread);
        void deleteDiscussionThread(DiscussionThreadPtr thread, bool deleteMessages);

        void insertDiscussionThreadMessage(DiscussionThreadMessagePtr message);
        void deleteDiscussionThreadMessage(DiscussionThreadMessagePtr message);

        void insertDiscussionTag(DiscussionTagPtr tag);
        void deleteDiscussionTag(DiscussionTagPtr tag);

        void insertDiscussionCategory(DiscussionCategoryPtr category);
        void deleteDiscussionCategory(DiscussionCategoryPtr category);

        void insertMessageComment(MessageCommentPtr comment);
        void deleteMessageComment(MessageCommentPtr comment);

        void insertPrivateMessage(PrivateMessagePtr comment);
        void deletePrivateMessage(PrivateMessagePtr comment);

        void insertAttachment(AttachmentPtr attachment);
        void deleteAttachment(AttachmentPtr attachment);

        void startBatchInsert();
        void stopBatchInsert();

    private:
        struct Impl;
        Impl* impl_;
    };

    typedef std::shared_ptr<EntityCollection> EntityCollectionRef;

    UserPtr anonymousUser();
    
    inline IdType anonymousUserId()
    {
        return Helpers::UuidString::empty;
    }

    inline bool isAnonymousUserId(IdTypeRef id)
    {
        return ! static_cast<bool>(id);
    }

    inline bool isAnonymousUser(const User& user)
    {
        return isAnonymousUserId(user.id());
    }

    inline bool isAnonymousUser(UserPtr user)
    {
        return isAnonymousUser(*user);
    }
}
