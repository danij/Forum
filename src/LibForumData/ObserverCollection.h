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

            virtual void onGetEntitiesCount(PerformedByType performedBy) override;

            virtual void onGetUsers(PerformedByType performedBy) override;
            virtual void onGetUserById(PerformedByType performedBy, const Forum::Entities::IdType& id) override;
            virtual void onGetUserByName(PerformedByType performedBy, const std::string& name) override;
            virtual void onGetDiscussionThreadCount(PerformedByType performedBy) override;
            virtual void onGetDiscussionThreads(PerformedByType performedBy) override;
            virtual void onGetDiscussionThreadById(PerformedByType performedBy,
                                                   const Forum::Entities::IdType& id) override;
            virtual void onGetDiscussionThreadsOfUser(PerformedByType performedBy,
                                                      const Forum::Entities::User& user) override;

            virtual void onAddNewUser(PerformedByType performedBy, const Forum::Entities::User& newUser) override;
            virtual void onChangeUser(PerformedByType performedBy, const Forum::Entities::User& user,
                                    Forum::Entities::User::ChangeType change) override;
            virtual void onDeleteUser(PerformedByType performedBy, const Forum::Entities::User& deletedUser) override;
            virtual void onAddNewDiscussionThread(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionThread& newThread) override;
            virtual void onChangeDiscussionThread(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionThread& thread,
                                                  Forum::Entities::DiscussionThread::ChangeType change) override;
            virtual void onDeleteDiscussionThread(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionThread& deletedThread) override;
            virtual void onAddNewDiscussionMessage(PerformedByType performedBy,
                                                   const Forum::Entities::DiscussionMessage& newMessage) override;
            virtual void onDeleteDiscussionMessage(PerformedByType performedBy,
                                                   const Forum::Entities::DiscussionMessage& deletedMessage) override;

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