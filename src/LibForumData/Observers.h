#pragma once

#include <memory>

#include "Entities.h"
#include "TypeHelpers.h"

namespace Forum
{
    namespace Repository
    {
        typedef const Forum::Entities::User& PerformedByType;

        struct ObserverContext_
        {
            PerformedByType performedBy;

            ObserverContext_(PerformedByType performedBy) : performedBy(performedBy)
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
            virtual void onGetUserById(ObserverContext context, const Forum::Entities::IdType& id) {};
            virtual void onGetUserByName(ObserverContext context, const std::string& name) {};

            virtual void onGetDiscussionThreads(ObserverContext context) {};
            virtual void onGetDiscussionThreadById(ObserverContext context, const Forum::Entities::IdType& id) {};
            virtual void onGetDiscussionThreadsOfUser(ObserverContext context, const Forum::Entities::User& user) {};

            virtual void onGetDiscussionThreadMessagesOfUser(ObserverContext context,
                                                             const Forum::Entities::User& user) {};
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

            virtual void onAddNewUser(ObserverContext context, const Forum::Entities::User& newUser) {};
            virtual void onChangeUser(ObserverContext context, const Forum::Entities::User& user,
                                      Forum::Entities::User::ChangeType change) {};
            virtual void onDeleteUser(ObserverContext context, const Forum::Entities::User& deletedUser) {};

            virtual void onAddNewDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& newThread) {};
            virtual void onChangeDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& thread,
                                                  Forum::Entities::DiscussionThread::ChangeType change) {};
            virtual void onDeleteDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& deletedThread) {};

            virtual void onAddNewDiscussionMessage(ObserverContext context,
                                                   const Forum::Entities::DiscussionMessage& newMessage) {};
            virtual void onDeleteDiscussionMessage(ObserverContext context,
                                                   const Forum::Entities::DiscussionMessage& deletedMessage) {};
        };

        typedef std::shared_ptr<AbstractWriteRepositoryObserver> WriteRepositoryObserverRef;

    }
}