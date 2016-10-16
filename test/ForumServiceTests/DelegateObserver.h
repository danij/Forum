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
            std::function<void(PerformedByType)> getUserCountAction;
            std::function<void(PerformedByType)> getUsersAction;
            std::function<void(PerformedByType, const std::string&)> getUsersByNameAction;

            std::function<void(PerformedByType, const Forum::Entities::User&)> addNewUserAction;
            std::function<void(PerformedByType, const Forum::Entities::User&,
                Forum::Entities::User::ChangeType)> changeUserAction;
            std::function<void(PerformedByType, const Forum::Entities::User&)> deleteUserAction;

            std::function<void(PerformedByType)> getDiscussionThreadCountAction;
            std::function<void(PerformedByType)> getDiscussionThreadsAction;
            std::function<void(PerformedByType, const Forum::Entities::IdType&)> getDiscussionThreadByIdAction;

            std::function<void(PerformedByType, const Forum::Entities::DiscussionThread&)> addNewDiscussionThreadAction;
            std::function<void(PerformedByType, const Forum::Entities::DiscussionThread&,
                    Forum::Entities::DiscussionThread::ChangeType)> changeDiscussionThreadAction;
            std::function<void(PerformedByType, const Forum::Entities::DiscussionThread&)> deleteDiscussionThreadAction;
        };

        class DelegateObserver final : public AbstractReadRepositoryObserver, public AbstractWriteRepositoryObserver,
                                       public DelegateObserverDelegates_, private boost::noncopyable
        {
        public:
            virtual void onGetUserCount(PerformedByType performedBy) override
            {
                if (getUserCountAction) getUserCountAction(performedBy);
            }
            virtual void onGetUsers(PerformedByType performedBy) override
            {
                if (getUsersAction) getUsersAction(performedBy);
            }
            virtual void onGetUserByName(PerformedByType performedBy, const std::string& name) override
            {
                if (getUsersByNameAction) getUsersByNameAction(performedBy, name);
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

            virtual void onGetDiscussionThreadCount(PerformedByType performedBy) override
            {
                if (getDiscussionThreadCountAction) getDiscussionThreadCountAction(performedBy);
            }
            virtual void onGetDiscussionThreads(PerformedByType performedBy) override
            {
                if (getDiscussionThreadsAction) getDiscussionThreadsAction(performedBy);
            }
            virtual void onGetDiscussionThreadById(PerformedByType performedBy, const Forum::Entities::IdType& id) override
            {
                if (getDiscussionThreadByIdAction) getDiscussionThreadByIdAction(performedBy, id);
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