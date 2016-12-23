#pragma once

#include "Entities.h"
#include "TypeHelpers.h"

#include <memory>

namespace Forum
{
    namespace Repository
    {
        typedef const Entities::User& PerformedByType;

        struct ObserverContext_
        {
            PerformedByType performedBy;
            Entities::Timestamp timestamp;

            ObserverContext_(PerformedByType performedBy, Entities::Timestamp timestamp) :
                    performedBy(performedBy), timestamp(timestamp)
            {
            }
        };

        typedef const ObserverContext_& ObserverContext;

        /**
         * Expanded by classes that want to be notified when a user performs a read action.
         * The class may be called from multiple threads so thread safety needs to be implemented.
         */
        class AbstractReadRepositoryObserver
        {
        public:
            DECLARE_INTERFACE_MANDATORY(AbstractReadRepositoryObserver);

            virtual void onGetEntitiesCount(ObserverContext context) {};

            virtual void onGetUsers(ObserverContext context) {};
            virtual void onGetUserById(ObserverContext context, const Entities::IdType& id) {};
            virtual void onGetUserByName(ObserverContext context, const std::string& name) {};

            virtual void onGetDiscussionThreads(ObserverContext context) {};
            virtual void onGetDiscussionThreadById(ObserverContext context, const Entities::IdType& id) {};
            virtual void onGetDiscussionThreadsOfUser(ObserverContext context, const Entities::User& user) {};

            virtual void onGetDiscussionThreadMessagesOfUser(ObserverContext context,
                                                             const Entities::User& user) {};
        };

        typedef std::shared_ptr<AbstractReadRepositoryObserver> ReadRepositoryObserverRef;

        /**
         * Expanded by classes that want to be notified when a user performs a write action.
         * The class may be called from multiple threads so thread safety needs to be implemented.
         */
        class AbstractWriteRepositoryObserver
        {
        public:
            DECLARE_INTERFACE_MANDATORY(AbstractWriteRepositoryObserver);

            virtual void onAddNewUser(ObserverContext context, const Entities::User& newUser) {};
            virtual void onChangeUser(ObserverContext context, const Entities::User& user,
                                      Entities::User::ChangeType change) {};
            virtual void onDeleteUser(ObserverContext context, const Entities::User& deletedUser) {};

            virtual void onAddNewDiscussionThread(ObserverContext context,
                                                  const Entities::DiscussionThread& newThread) {};
            virtual void onChangeDiscussionThread(ObserverContext context,
                                                  const Entities::DiscussionThread& thread,
                                                  Entities::DiscussionThread::ChangeType change) {};
            virtual void onDeleteDiscussionThread(ObserverContext context,
                                                  const Entities::DiscussionThread& deletedThread) {};

            virtual void onAddNewDiscussionMessage(ObserverContext context,
                                                   const Entities::DiscussionMessage& newMessage) {};
            virtual void onDeleteDiscussionMessage(ObserverContext context,
                                                   const Entities::DiscussionMessage& deletedMessage) {};
        };

        typedef std::shared_ptr<AbstractWriteRepositoryObserver> WriteRepositoryObserverRef;

    }
}