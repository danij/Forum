#pragma once

#include <functional>
#include <memory>

#include "CommandHandler.h"
#include "Observers.h"

namespace Forum
{
    namespace Repository
    {
        /**
         * Internal class used to hide public observer methods when accessed via DisposingDelegateObserver.operator->()
         */
        class DelegateObserverDelegates_
        {
        public:
            std::function<void(PerformedByType)> getEntitiesCountAction;

            std::function<void(PerformedByType)> getUsersAction;
            std::function<void(PerformedByType, const Forum::Entities::IdType&)> getUserByIdAction;
            std::function<void(PerformedByType, const std::string&)> getUserByNameAction;

            std::function<void(PerformedByType, const Forum::Entities::User&)> addNewUserAction;
            std::function<void(PerformedByType, const Forum::Entities::User&,
                Forum::Entities::User::ChangeType)> changeUserAction;
            std::function<void(PerformedByType, const Forum::Entities::User&)> deleteUserAction;

            std::function<void(PerformedByType)> getDiscussionThreadsAction;
            std::function<void(PerformedByType, const Forum::Entities::IdType&)> getDiscussionThreadByIdAction;
            std::function<void(PerformedByType, const Forum::Entities::User&)> getDiscussionThreadsOfUserAction;
            std::function<void(PerformedByType, const Forum::Entities::User&)> getDiscussionThreadMessagesOfUserAction;

            std::function<void(PerformedByType, const Forum::Entities::DiscussionThread&)> addNewDiscussionThreadAction;
            std::function<void(PerformedByType, const Forum::Entities::DiscussionThread&,
                    Forum::Entities::DiscussionThread::ChangeType)> changeDiscussionThreadAction;
            std::function<void(PerformedByType, const Forum::Entities::DiscussionThread&)> deleteDiscussionThreadAction;

            std::function<void(PerformedByType, const Forum::Entities::DiscussionMessage&)>
                addNewDiscussionMessageAction;
            std::function<void(PerformedByType, const Forum::Entities::DiscussionMessage&)>
                deleteDiscussionMessageAction;
        };

        class DelegateObserver final : public AbstractReadRepositoryObserver, public AbstractWriteRepositoryObserver,
                                       public DelegateObserverDelegates_, private boost::noncopyable
        {
        public:
            virtual void onGetEntitiesCount(PerformedByType performedBy) override
            {
                if (getEntitiesCountAction) getEntitiesCountAction(performedBy);
            }

            virtual void onGetUsers(PerformedByType performedBy) override
            {
                if (getUsersAction) getUsersAction(performedBy);
            }
            virtual void onGetUserById(PerformedByType performedBy, const Forum::Entities::IdType& id) override
            {
                if (getUserByIdAction) getUserByIdAction(performedBy, id);
            }
            virtual void onGetUserByName(PerformedByType performedBy, const std::string& name) override
            {
                if (getUserByNameAction) getUserByNameAction(performedBy, name);
            }

            virtual void onAddNewUser(PerformedByType performedBy, const Forum::Entities::User& newUser) override
            {
                if (addNewUserAction) addNewUserAction(performedBy, newUser);
            }
            virtual void onChangeUser(PerformedByType performedBy, const Forum::Entities::User& user,
                                      Forum::Entities::User::ChangeType change) override
            {
                if (changeUserAction) changeUserAction(performedBy, user, change);
            }
            virtual void onDeleteUser(PerformedByType performedBy, const Forum::Entities::User& deletedUser) override
            {
                if (deleteUserAction) deleteUserAction(performedBy, deletedUser);
            }

            virtual void onGetDiscussionThreads(PerformedByType performedBy) override
            {
                if (getDiscussionThreadsAction) getDiscussionThreadsAction(performedBy);
            }
            virtual void onGetDiscussionThreadById(PerformedByType performedBy,
                                                   const Forum::Entities::IdType& id) override
            {
                if (getDiscussionThreadByIdAction) getDiscussionThreadByIdAction(performedBy, id);
            }
            virtual void onGetDiscussionThreadsOfUser(PerformedByType performedBy,
                                                      const Forum::Entities::User& user) override
            {
                if (getDiscussionThreadsOfUserAction) getDiscussionThreadsOfUserAction(performedBy, user);
            }
            virtual void onGetDiscussionThreadMessagesOfUser(PerformedByType performedBy,
                                                      const Forum::Entities::User& user) override
            {
                if (getDiscussionThreadMessagesOfUserAction) getDiscussionThreadMessagesOfUserAction(performedBy, user);
            }

            virtual void onAddNewDiscussionThread(PerformedByType performedBy,
                                                const Forum::Entities::DiscussionThread& newThread) override
            {
                if (addNewDiscussionThreadAction) addNewDiscussionThreadAction(performedBy, newThread);
            }
            virtual void onChangeDiscussionThread(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionThread& thread,
                                                  Forum::Entities::DiscussionThread::ChangeType change) override
            {
                if (changeDiscussionThreadAction) changeDiscussionThreadAction(performedBy, thread, change);
            }
            virtual void onDeleteDiscussionThread(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionThread& deletedThread) override
            {
                if (deleteDiscussionThreadAction) deleteDiscussionThreadAction(performedBy, deletedThread);
            }

            virtual void onAddNewDiscussionMessage(PerformedByType performedBy,
                                                const Forum::Entities::DiscussionMessage& newMessage) override
            {
                if (addNewDiscussionMessageAction) addNewDiscussionMessageAction(performedBy, newMessage);
            }
            virtual void onDeleteDiscussionMessage(PerformedByType performedBy,
                                                  const Forum::Entities::DiscussionMessage& deletedMessage) override
            {
                if (deleteDiscussionMessageAction) deleteDiscussionMessageAction(performedBy, deletedMessage);
            }

        };

        struct DisposingDelegateObserver
        {
            DisposingDelegateObserver(Forum::Commands::CommandHandler& handler)
            {
                observer_ = std::make_shared<DelegateObserver>();
                readRepository_ = handler.getReadRepository();
                writeRepository_ = handler.getWriteRepository();

                readRepository_->addObserver(observer_);
                writeRepository_->addObserver(observer_);
            }

            ~DisposingDelegateObserver()
            {
                readRepository_->removeObserver(observer_);
                writeRepository_->removeObserver(observer_);
            }

            DelegateObserverDelegates_& operator*() const
            {
                return *this->operator->();
            }

            DelegateObserverDelegates_* operator->() const
            {
                return observer_.get();
            }

        private:
            std::shared_ptr<DelegateObserver> observer_;
            ReadRepositoryRef readRepository_;
            WriteRepositoryRef writeRepository_;
        };
    }
}