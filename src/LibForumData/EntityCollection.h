#pragma once

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
        struct EntityCollection : public UserCollectionBase, public DiscussionThreadCollectionBase,
                                  public DiscussionThreadMessageCollectionBase, public MessageCommentCollectionBase,
                                  public DiscussionTagCollectionBase, public DiscussionCategoryCollectionBase
        {
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            virtual UserRef deleteUser(UserCollection::iterator iterator) override;

            /**
             * Enables a safe modification of a discussion thread instance,
             * also taking other collections in which the thread might be registered into account
             */
            virtual void modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                                const std::function<void(DiscussionThread&)>& modifyFunction) override;
            /**
             * Safely deletes a discussion thread instance,
             * also taking other collections in which the thread might be registered into account
             */
            virtual DiscussionThreadRef deleteDiscussionThread(DiscussionThreadCollection::iterator iterator) override;

            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            virtual void modifyDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator,
                                                 const std::function<void(DiscussionThreadMessage&)>& modifyFunction) override;
            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            virtual DiscussionThreadMessageRef deleteDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator) override;

            /**
            * Safely deletes a discussion tag instance, removing it from all indexes it is registered in
            */
            virtual DiscussionTagRef deleteDiscussionTag(DiscussionTagCollection::iterator iterator) override;

            /**
            * Safely deletes a discussion category instance, removing it from all indexes it is registered in
            */
            virtual DiscussionCategoryRef deleteDiscussionCategory(DiscussionCategoryCollection::iterator iterator) override;

            auto& notifyTagChange() { return notifyTagChange_; }
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

        extern const UserRef AnonymousUser;
        extern const IdType AnonymousUserId;
    }
}
