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

            virtual void onGetEntitiesCount(ObserverContext context) override;

            virtual void onGetUsers(ObserverContext context) override;
            virtual void onGetUserById(ObserverContext context, const Forum::Entities::IdType& id) override;
            virtual void onGetUserByName(ObserverContext context, const std::string& name) override;
            virtual void onGetDiscussionThreads(ObserverContext context) override;
            virtual void onGetDiscussionThreadById(ObserverContext context,
                                                   const Forum::Entities::IdType& id) override;
            virtual void onGetDiscussionThreadsOfUser(ObserverContext context,
                                                      const Forum::Entities::User& user) override;
            virtual void onGetDiscussionThreadMessagesOfUser(ObserverContext context,
                                                             const Forum::Entities::User& user) override;

            virtual void onAddNewUser(ObserverContext context, const Forum::Entities::User& newUser) override;
            virtual void onChangeUser(ObserverContext context, const Forum::Entities::User& user,
                                    Forum::Entities::User::ChangeType change) override;
            virtual void onDeleteUser(ObserverContext context, const Forum::Entities::User& deletedUser) override;
            virtual void onAddNewDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& newThread) override;
            virtual void onChangeDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& thread,
                                                  Forum::Entities::DiscussionThread::ChangeType change) override;
            virtual void onDeleteDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& deletedThread) override;
            virtual void onAddNewDiscussionMessage(ObserverContext context,
                                                   const Forum::Entities::DiscussionMessage& newMessage) override;
            virtual void onDeleteDiscussionMessage(ObserverContext context,
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