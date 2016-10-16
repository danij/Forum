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
        };

        class DelegateObserver final : public AbstractReadRepositoryObserver, public AbstractWriteRepositoryObserver,
                                       public DelegateObserverDelegates_, private boost::noncopyable
        {
        public:
            virtual void getUserCount(PerformedByType performedBy) override
            {
                if (getUserCountAction) getUserCountAction(performedBy);
            }
            virtual void getUsers(PerformedByType performedBy) override
            {
                if (getUsersAction) getUsersAction(performedBy);
            }
            virtual void getUserByName(PerformedByType performedBy, const std::string& name) override
            {
                if (getUsersByNameAction) getUsersByNameAction(performedBy, name);
            }

            virtual void addNewUser(PerformedByType performedBy, const Forum::Entities::User& newUser) override
            {
                if (addNewUserAction) addNewUserAction(performedBy, newUser);
            }
            virtual void changeUser(PerformedByType performedBy, const Forum::Entities::User& user,
                                    Forum::Entities::User::ChangeType change) override
            {
                if (changeUserAction) changeUserAction(performedBy, user, change);
            }
            virtual void deleteUser(PerformedByType performedBy, const Forum::Entities::User& deletedUser) override
            {
                if (deleteUserAction) deleteUserAction(performedBy, deletedUser);
            }

            virtual void getDiscussionThreadCount(PerformedByType performedBy) override
            {
                if (getDiscussionThreadCountAction) getDiscussionThreadCountAction(performedBy);
            }
            virtual void getDiscussionThreads(PerformedByType performedBy) override
            {
                if (getDiscussionThreadsAction) getDiscussionThreadsAction(performedBy);
            }
            virtual void getDiscussionThreadById(PerformedByType performedBy, const Forum::Entities::IdType& id) override
            {
                if (getDiscussionThreadByIdAction) getDiscussionThreadByIdAction(performedBy, id);
            }

            virtual void addNewDiscussionThread(PerformedByType performedBy,
                                                const Forum::Entities::DiscussionThread& newThread) override
            {
                if (addNewDiscussionThreadAction) addNewDiscussionThread(performedBy, newThread);
            }
            virtual void changeDiscussionThread(PerformedByType performedBy,
                                                const Forum::Entities::DiscussionThread& thread,
                                                Forum::Entities::DiscussionThread::ChangeType change) override
            {
                if (changeDiscussionThreadAction) changeDiscussionThreadAction(performedBy, thread, change);
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