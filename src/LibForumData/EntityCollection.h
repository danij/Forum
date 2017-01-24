#pragma once

#include "EntityDiscussionThreadMessageCollectionBase.h"
#include "EntityDiscussionThreadCollectionBase.h"
#include "EntityDiscussionTagCollectionBase.h"
#include "EntityUserCollectionBase.h"
#include "Entities.h"

#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct EntityCollection : public UserCollectionBase,
                                  public DiscussionThreadCollectionBase, public DiscussionThreadMessageCollectionBase,
                                  public DiscussionTagCollectionBase
        {
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            virtual void deleteUser(UserCollection::iterator iterator) override;

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
            virtual void deleteDiscussionThread(DiscussionThreadCollection::iterator iterator) override;

            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            virtual void modifyDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator,
                                                 const std::function<void(DiscussionThreadMessage&)>& modifyFunction) override;
            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            virtual void deleteDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator) override;
        };

        extern const UserRef AnonymousUser;
        extern const IdType AnonymousUserId;
    }
}
