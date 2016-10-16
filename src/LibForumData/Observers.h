#pragma once

#include <memory>

#include "Entities.h"

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
            virtual ~AbstractReadRepositoryObserver() = default;

            virtual void getUserCount(PerformedByType performedBy) {};
            virtual void getUsers(PerformedByType performedBy) {};
            virtual void getUserByName(PerformedByType performedBy, const std::string& name) {};

            virtual void getDiscussionThreadCount(PerformedByType performedBy) {};
            virtual void getDiscussionThreads(PerformedByType performedBy) {};
            virtual void getDiscussionThreadById(PerformedByType performedBy, const Forum::Entities::IdType& id) {};
        };

        typedef std::shared_ptr<AbstractReadRepositoryObserver> ReadRepositoryObserverRef;

        /**
         * Expanded by classes that want to be notified when a user performs a write action.
         * The class may be called from multiple threads so thread safety needs to be implemented.
         */
        class AbstractWriteRepositoryObserver
        {
        public:
            virtual ~AbstractWriteRepositoryObserver() = default;

            virtual void addNewUser(PerformedByType performedBy, const Forum::Entities::User& newUser) {};
            virtual void changeUser(PerformedByType performedBy, const Forum::Entities::User& user,
                                    Forum::Entities::User::ChangeType change) {};
            virtual void deleteUser(PerformedByType performedBy, const Forum::Entities::User& deletedUser) {};

            virtual void addNewDiscussionThread(PerformedByType performedBy,
                                                const Forum::Entities::DiscussionThread& newUser) {};
            virtual void changeDiscussionThread(PerformedByType performedBy,
                                                const Forum::Entities::DiscussionThread& thread,
                                                Forum::Entities::DiscussionThread::ChangeType change) {};

        };

        typedef std::shared_ptr<AbstractWriteRepositoryObserver> WriteRepositoryObserverRef;

    }
}