#pragma once

#include <mutex>
#include <shared_mutex>
#include <vector>

#include <boost/core/noncopyable.hpp>

#include "Observers.h"

namespace Forum
{
    namespace Repository
    {
        /**
         * Keeps a collection of observers and notifies each one when an action occurs.
         * Adding/removing observers to the collection is thread safe.
         */
        class ObserverCollection final : public AbstractReadRepositoryObserver, public AbstractWriteRepositoryObserver,
                                         private boost::noncopyable
        {
        public:
            void addObserver(const ReadRepositoryObserverRef& observer);
            void addObserver(const WriteRepositoryObserverRef& observer);
            void removeObserver(const ReadRepositoryObserverRef& observer);
            void removeObserver(const WriteRepositoryObserverRef& observer);

            virtual void getUserCount(PerformedByType performedBy) override;
            virtual void getUsers(PerformedByType performedBy) override;
            virtual void getUserByName(PerformedByType performedBy, const std::string& name) override;
            virtual void getDiscussionThreadCount(PerformedByType performedBy) override;
            virtual void getDiscussionThreads(PerformedByType performedBy) override;

            virtual void addNewUser(PerformedByType performedBy, const Forum::Entities::User& newUser) override;
            virtual void changeUser(PerformedByType performedBy, const Forum::Entities::User& newUser,
                                    Forum::Entities::User::ChangeType change) override;
            virtual void deleteUser(PerformedByType performedBy, const Forum::Entities::User& deletedUser) override;

        private:
            std::vector<ReadRepositoryObserverRef> readObservers_;
            std::vector<WriteRepositoryObserverRef> writeObservers_;
            /**
             * Mutex used to synchronize read/write of vectors
             */
            std::shared_timed_mutex mutex_;
        };
    }
}