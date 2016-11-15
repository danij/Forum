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
            std::function<void(ObserverContext)> getEntitiesCountAction;

            std::function<void(ObserverContext)> getUsersAction;
            std::function<void(ObserverContext, const Forum::Entities::IdType&)> getUserByIdAction;
            std::function<void(ObserverContext, const std::string&)> getUserByNameAction;

            std::function<void(ObserverContext, const Forum::Entities::User&)> addNewUserAction;
            std::function<void(ObserverContext, const Forum::Entities::User&,
                Forum::Entities::User::ChangeType)> changeUserAction;
            std::function<void(ObserverContext, const Forum::Entities::User&)> deleteUserAction;

            std::function<void(ObserverContext)> getDiscussionThreadsAction;
            std::function<void(ObserverContext, const Forum::Entities::IdType&)> getDiscussionThreadByIdAction;
            std::function<void(ObserverContext, const Forum::Entities::User&)> getDiscussionThreadsOfUserAction;
            std::function<void(ObserverContext, const Forum::Entities::User&)> getDiscussionThreadMessagesOfUserAction;

            std::function<void(ObserverContext, const Forum::Entities::DiscussionThread&)> addNewDiscussionThreadAction;
            std::function<void(ObserverContext, const Forum::Entities::DiscussionThread&,
                    Forum::Entities::DiscussionThread::ChangeType)> changeDiscussionThreadAction;
            std::function<void(ObserverContext, const Forum::Entities::DiscussionThread&)> deleteDiscussionThreadAction;

            std::function<void(ObserverContext, const Forum::Entities::DiscussionMessage&)>
                addNewDiscussionMessageAction;
            std::function<void(ObserverContext, const Forum::Entities::DiscussionMessage&)>
                deleteDiscussionMessageAction;
        };

        class DelegateObserver final : public AbstractReadRepositoryObserver, public AbstractWriteRepositoryObserver,
                                       public DelegateObserverDelegates_, private boost::noncopyable
        {
        public:
            virtual void onGetEntitiesCount(ObserverContext context) override
            {
                if (getEntitiesCountAction) getEntitiesCountAction(context);
            }

            virtual void onGetUsers(ObserverContext context) override
            {
                if (getUsersAction) getUsersAction(context);
            }
            virtual void onGetUserById(ObserverContext context, const Forum::Entities::IdType& id) override
            {
                if (getUserByIdAction) getUserByIdAction(context, id);
            }
            virtual void onGetUserByName(ObserverContext context, const std::string& name) override
            {
                if (getUserByNameAction) getUserByNameAction(context, name);
            }

            virtual void onAddNewUser(ObserverContext context, const Forum::Entities::User& newUser) override
            {
                if (addNewUserAction) addNewUserAction(context, newUser);
            }
            virtual void onChangeUser(ObserverContext context, const Forum::Entities::User& user,
                                      Forum::Entities::User::ChangeType change) override
            {
                if (changeUserAction) changeUserAction(context, user, change);
            }
            virtual void onDeleteUser(ObserverContext context, const Forum::Entities::User& deletedUser) override
            {
                if (deleteUserAction) deleteUserAction(context, deletedUser);
            }

            virtual void onGetDiscussionThreads(ObserverContext context) override
            {
                if (getDiscussionThreadsAction) getDiscussionThreadsAction(context);
            }
            virtual void onGetDiscussionThreadById(ObserverContext context,
                                                   const Forum::Entities::IdType& id) override
            {
                if (getDiscussionThreadByIdAction) getDiscussionThreadByIdAction(context, id);
            }
            virtual void onGetDiscussionThreadsOfUser(ObserverContext context,
                                                      const Forum::Entities::User& user) override
            {
                if (getDiscussionThreadsOfUserAction) getDiscussionThreadsOfUserAction(context, user);
            }
            virtual void onGetDiscussionThreadMessagesOfUser(ObserverContext context,
                                                             const Forum::Entities::User& user) override
            {
                if (getDiscussionThreadMessagesOfUserAction) getDiscussionThreadMessagesOfUserAction(context, user);
            }

            virtual void onAddNewDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& newThread) override
            {
                if (addNewDiscussionThreadAction) addNewDiscussionThreadAction(context, newThread);
            }
            virtual void onChangeDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& thread,
                                                  Forum::Entities::DiscussionThread::ChangeType change) override
            {
                if (changeDiscussionThreadAction) changeDiscussionThreadAction(context, thread, change);
            }
            virtual void onDeleteDiscussionThread(ObserverContext context,
                                                  const Forum::Entities::DiscussionThread& deletedThread) override
            {
                if (deleteDiscussionThreadAction) deleteDiscussionThreadAction(context, deletedThread);
            }

            virtual void onAddNewDiscussionMessage(ObserverContext context,
                                                   const Forum::Entities::DiscussionMessage& newMessage) override
            {
                if (addNewDiscussionMessageAction) addNewDiscussionMessageAction(context, newMessage);
            }
            virtual void onDeleteDiscussionMessage(ObserverContext context,
                                                  const Forum::Entities::DiscussionMessage& deletedMessage) override
            {
                if (deleteDiscussionMessageAction) deleteDiscussionMessageAction(context, deletedMessage);
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