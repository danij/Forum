#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityDiscussionThreadMessageCollectionBase.h"
#include "EntityMessageCommentCollectionBase.h"
#include "EntityDiscussionThreadCollectionBase.h"
#include "EntityDiscussionTagCollectionBase.h"
#include "EntityDiscussionCategoryCollectionBase.h"
#include "EntityUserCollectionBase.h"
#include "Entities.h"

#include <memory>
#include <functional>

namespace Forum
{
    namespace Entities
    {
        /**
         * Stores references to all entities present in memory
         * Upon deleting an entitie, the collection also removes the entity from any other collection if might have been part of
         */
        struct EntityCollection : public UserCollectionBase<HashIndexForId>,
                                  public DiscussionThreadCollectionBase<HashIndexForId>,
                                  public DiscussionThreadMessageCollectionBase<HashIndexForId>, 
                                  public MessageCommentCollectionBase<HashIndexForId>,
                                  public DiscussionTagCollectionBase<HashIndexForId>,
                                  public DiscussionCategoryCollectionBase<HashIndexForId>,
                                  public Authorization::ForumWidePrivilegeStore
        {
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

            EntityCollection()
            {
                notifyTagChange_ = [this](auto& tag)
                {
                    this->modifyDiscussionTagById(tag.id(), [](auto&) {});
                };
                notifyCategoryChange_ = [this](auto& category)
                {
                    this->modifyDiscussionCategoryById(category.id(), [](auto&) {});
                };
            }

        private:
            DiscussionTag::NotifyChangeActionType notifyTagChange_;
            DiscussionCategory::NotifyChangeActionType notifyCategoryChange_;
        };
        typedef std::shared_ptr<EntityCollection> EntityCollectionRef;

        extern const UserRef AnonymousUser;
        extern const IdType AnonymousUserId;
    }
}
