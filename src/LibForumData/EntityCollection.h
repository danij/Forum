#pragma once

#include "EntityDiscussionMessageCollectionBase.h"
#include "EntityDiscussionThreadCollectionBase.h"
#include "EntityUserCollectionBase.h"
#include "Entities.h"

#include <string>
#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct EntityCollection : public UserCollectionBase,
                                  public DiscussionThreadCollectionBase, public DiscussionMessageCollectionBase
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
            virtual void modifyDiscussionMessage(DiscussionMessageCollection::iterator iterator,
                                                 const std::function<void(DiscussionMessage&)>& modifyFunction) override;
            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            virtual void deleteDiscussionMessage(DiscussionMessageCollection::iterator iterator) override;
        };

        extern const UserRef AnonymousUser;
    }
}
