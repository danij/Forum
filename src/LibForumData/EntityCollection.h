#pragma once

#include "AuthorizationPrivileges.h"
#include "AuthorizationGrantedPrivilegeStore.h"

#include "EntityUserCollection.h"
#include "EntityDiscussionThreadCollection.h"
#include "EntityDiscussionThreadMessageCollection.h"
#include "EntityDiscussionTagCollection.h"
#include "EntityDiscussionCategoryCollection.h"
#include "EntityMessageCommentCollection.h"

#include <memory>
#include <functional>

namespace Forum
{
    namespace Entities
    {
        /**
         * Stores references to all entities present in memory
         * Upon deleting an entity, the collection also removes the entity from any other collection if might have been part of
         */
        class EntityCollection final : public UserCollectionBase<HashIndexForId>,
                                  public DiscussionThreadCollectionBase<HashIndexForId>,
                                  public DiscussionThreadMessageCollectionBase<HashIndexForId>,
                                  public MessageCommentCollectionBase<HashIndexForId>,
                                  public DiscussionTagCollectionBase<HashIndexForId>,
                                  public DiscussionCategoryCollectionBase<HashIndexForId>,
                                  public Authorization::ForumWidePrivilegeStore
        {
        public:
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            UserRef deleteUser(UserIdIteratorType iterator) override;

            /**
             * Enables a safe modification of a discussion thread instance,
             * also taking other collections in which the thread might be registered into account
             */
            void modifyDiscussionThread(ThreadIdIteratorType iterator,
                                        std::function<void(DiscussionThread&)>&& modifyFunction) override;
            /**
             * Safely deletes a discussion thread instance,
             * also taking other collections in which the thread might be registered into account
             */
            DiscussionThreadRef deleteDiscussionThread(ThreadIdIteratorType iterator) override;

            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            void modifyDiscussionThreadMessage(MessageIdIteratorType iterator,
                                               std::function<void(DiscussionThreadMessage&)>&& modifyFunction) override;
            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            DiscussionThreadMessageRef deleteDiscussionThreadMessage(MessageIdIteratorType iterator) override;

            /**
            * Safely deletes a discussion tag instance, removing it from all indexes it is registered in
            */
            DiscussionTagRef deleteDiscussionTag(TagIdIteratorType iterator) override;

            /**
            * Safely deletes a discussion category instance, removing it from all indexes it is registered in
            */
            DiscussionCategoryRef deleteDiscussionCategory(CategoryIdIteratorType iterator) override;

            auto& notifyTagChange()      { return notifyTagChange_; }
            auto& notifyCategoryChange() { return notifyCategoryChange_; }

                        
            const auto& grantedPrivileges() const { return grantedPrivileges_; }
                  auto& grantedPrivileges()       { return grantedPrivileges_; }

            //new
            EntityCollection();
            ~EntityCollection();

            std::unique_ptr<User>* getUserPoolRoot();
            std::unique_ptr<DiscussionThread>* getDiscussionThreadPoolRoot();
            std::unique_ptr<DiscussionThreadMessage>* getDiscussionThreadMessagePoolRoot();
            std::unique_ptr<DiscussionTag>* getDiscussionTagPoolRoot();
            std::unique_ptr<DiscussionCategory>* getDiscussionCategoryPoolRoot();
            std::unique_ptr<MessageComment>* getMessageCommentPoolRoot();

            UserPtr createAndAddUser(IdType id, Timestamp created);
            DiscussionThreadPtr createAndAddDiscussionThread(/*EXTRA PARAMETERS*/);
            DiscussionThreadMessagePtr createAndAddDiscussionThreadMessage();
            DiscussionTagPtr createAndAddDiscussionTag();
            DiscussionCategoryPtr createAndAddDiscussionCategory();

            const UserCollection& users() const;
            const DiscussionThreadCollectionWithHashedId& threads() const;
            const DiscussionThreadMessageCollection& threadMessages() const;
            const DiscussionTagCollection& tags() const;
            const DiscussionCategoryCollection& categories() const;
            const MessageCommentCollection& messageComments() const;

        private:
            DiscussionTag::NotifyChangeActionType notifyTagChange_;
            DiscussionCategory::NotifyChangeActionType notifyCategoryChange_;
            Authorization::GrantedPrivilegeStore grantedPrivileges_;

            struct Impl;
            Impl* impl_;
        };
        typedef std::shared_ptr<EntityCollection> EntityCollectionRef;

        extern const UserPtr AnonymousUser;
        extern const IdType AnonymousUserId;
    }
}
