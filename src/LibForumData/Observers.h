#pragma once

#include "Entities.h"
#include "TypeHelpers.h"
#include "ContextProviders.h"

#include <memory>

namespace Forum
{
    namespace Repository
    {
        typedef const Entities::User& PerformedByType;

        struct ObserverContext_
        {
            const PerformedByType performedBy;
            const Entities::Timestamp timestamp;
            const Context::DisplayContext displayContext;

            ObserverContext_(PerformedByType performedBy, Entities::Timestamp timestamp, 
                            Context::DisplayContext displayContext) :
                    performedBy(performedBy), timestamp(timestamp), displayContext(displayContext)
            {
            }
        };

        //Do not polute all observer methods with const Struct&
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

            virtual void onGetDiscussionTags(ObserverContext context) {};
            virtual void onGetDiscussionThreadsWithTag(ObserverContext context, const Entities::DiscussionTag& tag) {};
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
            virtual void onMergeDiscussionThreads(ObserverContext context,
                                                  const Entities::DiscussionThread& fromThread,
                                                  const Entities::DiscussionThread& toThread) {};
            virtual void onMoveDiscussionThreadMessage(ObserverContext context,
                                                       const Entities::DiscussionMessage& message,
                                                       const Entities::DiscussionThread& intoThread) {};

            virtual void onAddNewDiscussionMessage(ObserverContext context,
                                                   const Entities::DiscussionMessage& newMessage) {};
            virtual void onDeleteDiscussionMessage(ObserverContext context,
                                                   const Entities::DiscussionMessage& deletedMessage) {};

            virtual void onAddNewDiscussionTag(ObserverContext context,
                                               const Entities::DiscussionTag& newTag) {};
            virtual void onChangeDiscussionTag(ObserverContext context,
                                               const Entities::DiscussionTag& tag,
                                               Entities::DiscussionTag::ChangeType change) {};
            virtual void onDeleteDiscussionTag(ObserverContext context,
                                               const Entities::DiscussionTag& deletedTag) {};
            virtual void onAddDiscussionTagToThread(ObserverContext context,
                                                    const Entities::DiscussionTag& tag,
                                                    const Entities::DiscussionThread& thread) {};
            virtual void onRemoveDiscussionTagFromThread(ObserverContext context,
                                                         const Entities::DiscussionTag& tag,
                                                         const Entities::DiscussionThread& thread) {};
            virtual void onMergeDiscussionTags(ObserverContext context,
                                               const Entities::DiscussionTag& fromTag,
                                               const Entities::DiscussionTag& toTag) {};
        };

        typedef std::shared_ptr<AbstractWriteRepositoryObserver> WriteRepositoryObserverRef;

    }
}