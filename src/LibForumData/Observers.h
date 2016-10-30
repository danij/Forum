#pragma once

#include <memory>

#include "Entities.h"
#include "TypeHelpers.h"

namespace Forum
{
    namespace Repository
    {
        typedef const Forum::Entities::User& PerformedByType;

        /**
         * Expanded by classes that want to be notified when a user performs a read action.
         * The class may be called from multiple threads so thread safety needs to be implemented.
         */
        class AbstractReadRepositoryObserver
        {
        public:
            DECLARE_INTERFACE_MANDATORY(AbstractReadRepositoryObserver);

            virtual void onGetEntitiesCount(PerformedByType performedBy) {};

            virtual void onGetUsers(PerformedByType performedBy) {};
            virtual void onGetUserById(PerformedByType performedBy, const Forum::Entities::IdType& id) {};
            virtual void onGetUserByName(PerformedByType performedBy, const std::string& name) {};

            virtual void onGetDiscussionThreadCount(PerformedByType performedBy) {};
            virtual void onGetDiscussionThreads(PerformedByType performedBy) {};
            virtual void onGetDiscussionThreadById(PerformedByType performedBy, const Forum::Entities::IdType& id) {};
            virtual void onGetDiscussionThreadsOfUser(PerformedByType performedBy, const Forum::Entities::User& user) {};
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

            virtual void onAddNewUser(PerformedByType performedBy, const Forum::Entities::User& newUser) {};
            virtual void onChangeUser(PerformedByType performedBy, const Forum::Entities::User& user,
                                      Forum::Entities::User::ChangeType change) {};
            virtual void onDeleteUser(PerformedByType performedBy, const Forum::Entities::User& deletedUser) {};

            virtual void onAddNewDiscussionThread(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionThread& newThread) {};
            virtual void onChangeDiscussionThread(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionThread& thread,
                                                  Forum::Entities::DiscussionThread::ChangeType change) {};
            virtual void onDeleteDiscussionThread(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionThread& deletedThread) {};

            virtual void onAddNewDiscussionMessage(PerformedByType performedBy,
                                                   const Forum::Entities::DiscussionMessage& newMessage) {};
            virtual void onDeleteDiscussionMessage(PerformedByType performedBy,
                                                   const Forum::Entities::DiscussionMessage& deletedMessage) {};
        };

        typedef std::shared_ptr<AbstractWriteRepositoryObserver> WriteRepositoryObserverRef;

    }
}