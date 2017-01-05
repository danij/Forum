#pragma once

#include "Observers.h"

#include <boost/core/noncopyable.hpp>

#include <shared_mutex>
#include <vector>

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
            virtual void onGetUserById(ObserverContext context, const Entities::IdType& id) override;
            virtual void onGetUserByName(ObserverContext context, const std::string& name) override;
            virtual void onGetDiscussionThreads(ObserverContext context) override;
            virtual void onGetDiscussionThreadById(ObserverContext context,
                                                   const Entities::IdType& id) override;
            virtual void onGetDiscussionThreadsOfUser(ObserverContext context,
                                                      const Entities::User& user) override;
            virtual void onGetDiscussionThreadMessagesOfUser(ObserverContext context,
                                                             const Entities::User& user) override;
            virtual void onGetDiscussionTags(ObserverContext context) override;

            virtual void onAddNewUser(ObserverContext context, const Entities::User& newUser) override;
            virtual void onChangeUser(ObserverContext context, const Entities::User& user,
                                      Entities::User::ChangeType change) override;
            virtual void onDeleteUser(ObserverContext context, const Entities::User& deletedUser) override;
            virtual void onAddNewDiscussionThread(ObserverContext context,
                                                  const Entities::DiscussionThread& newThread) override;
            virtual void onChangeDiscussionThread(ObserverContext context,
                                                  const Entities::DiscussionThread& thread,
                                                  Entities::DiscussionThread::ChangeType change) override;
            virtual void onDeleteDiscussionThread(ObserverContext context,
                                                  const Entities::DiscussionThread& deletedThread) override;
            virtual void onMergeDiscussionThreads(ObserverContext context,
                                                  const Entities::DiscussionThread& fromThread,
                                                  const Entities::DiscussionThread& toThread) override;
            virtual void onMoveDiscussionThreadMessage(ObserverContext context,
                                                       const Entities::DiscussionMessage& message,
                                                       const Entities::DiscussionThread& intoThread) override;
            virtual void onAddNewDiscussionMessage(ObserverContext context,
                                                   const Entities::DiscussionMessage& newMessage) override;
            virtual void onDeleteDiscussionMessage(ObserverContext context,
                                                   const Entities::DiscussionMessage& deletedMessage) override;
            virtual void onAddNewDiscussionTag(ObserverContext context,
                                               const Entities::DiscussionTag& newTag) override;
            virtual void onChangeDiscussionTag(ObserverContext context,
                                               const Entities::DiscussionTag& tag,
                                               Entities::DiscussionTag::ChangeType change) override;
            virtual void onDeleteDiscussionTag(ObserverContext context,
                                               const Entities::DiscussionTag& deletedTag) override;
            virtual void onAddDiscussionTagToThread(ObserverContext context,
                                                    const Entities::DiscussionTag& tag,
                                                    const Entities::DiscussionThread& thread) override;
            virtual void onRemoveDiscussionTagFromThread(ObserverContext context,
                                                         const Entities::DiscussionTag& tag,
                                                         const Entities::DiscussionThread& thread) override;
            virtual void onGetDiscussionThreadsWithTag(ObserverContext context, 
                                                       const Entities::DiscussionTag& tag) override;
            virtual void onMergeDiscussionTags(ObserverContext context,
                                               const Entities::DiscussionTag& fromTag,
                                               const Entities::DiscussionTag& toTag) override;

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