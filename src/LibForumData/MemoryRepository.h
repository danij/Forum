#pragma once

#include <boost/core/noncopyable.hpp>

#include "EntityCollection.h"
#include "ObserverCollection.h"
#include "Repository.h"
#include "ResourceGuard.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepository final : public IReadRepository, public IWriteRepository, private boost::noncopyable
        {
        public:
            MemoryRepository();

            virtual void addObserver(const ReadRepositoryObserverRef& observer) override;
            virtual void addObserver(const WriteRepositoryObserverRef& observer) override;
            virtual void removeObserver(const ReadRepositoryObserverRef& observer) override;
            virtual void removeObserver(const WriteRepositoryObserverRef& observer) override;

            virtual void getEntitiesCount(std::ostream& output) const override;

            virtual void getUsersByName(std::ostream& output) const override;
            virtual void getUsersByCreated(std::ostream& output) const override;
            virtual void getUsersByLastSeen(std::ostream& output) const override;

            virtual void getUserById(const Forum::Entities::IdType& id, std::ostream& output) const override;
            virtual void getUserByName(const std::string& name, std::ostream& output) const override;

            virtual void addNewUser(const std::string& name, std::ostream& output) override;
            virtual void changeUserName(const Forum::Entities::IdType& id, const std::string& newName,
                                        std::ostream& output) override;
            virtual void deleteUser(const Forum::Entities::IdType& id, std::ostream& output) override;

            virtual void getDiscussionThreadsByName(std::ostream& output) const override;
            virtual void getDiscussionThreadsByCreatedAscending(std::ostream& output) const override;
            virtual void getDiscussionThreadsByCreatedDescending(std::ostream& output) const override;
            virtual void getDiscussionThreadsByLastUpdatedAscending(std::ostream& output) const override;
            virtual void getDiscussionThreadsByLastUpdatedDescending(std::ostream& output) const override;
            virtual void getDiscussionThreadById(const Forum::Entities::IdType& id, std::ostream& output) const override;

            virtual void getDiscussionThreadsOfUserByName(const Forum::Entities::IdType& id,
                                                          std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByCreatedAscending(const Forum::Entities::IdType& id,
                                                                      std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByCreatedDescending(const Forum::Entities::IdType& id,
                                                                       std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByLastUpdatedAscending(const Forum::Entities::IdType& id,
                                                                          std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByLastUpdatedDescending(const Forum::Entities::IdType& id,
                                                                           std::ostream& output) const override;

            virtual void addNewDiscussionThread(const std::string& name, std::ostream& output) override;
            virtual void changeDiscussionThreadName(const Forum::Entities::IdType& id, const std::string& newName,
                                                    std::ostream& output) override;
            virtual void deleteDiscussionThread(const Forum::Entities::IdType& id, std::ostream& output) override;

            virtual void addNewDiscussionMessageInThread(const Forum::Entities::IdType& threadId,
                                                         const std::string& content, std::ostream& output) override;
            virtual void deleteDiscussionMessage(const Forum::Entities::IdType& id, std::ostream& output) override;

        private:
            friend struct PerformedByWithLastSeenUpdateGuard;

            void getDiscussionThreadsByCreated(bool ascending, std::ostream& output) const;
            void getDiscussionThreadsByLastUpdated(bool ascending, std::ostream& output) const;

            void getDiscussionThreadsOfUserByCreated(bool ascending, const Forum::Entities::IdType& id,
                                                     std::ostream& output) const;
            void getDiscussionThreadsOfUserByLastUpdated(bool ascending, const Forum::Entities::IdType& id,
                                                         std::ostream& output) const;

            Forum::Helpers::ResourceGuard<Forum::Entities::EntityCollection> collection_;
            mutable ObserverCollection observers_;
        };

        /**
         * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
         * The update is performed on the spot if a write lock is held or
         * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
         * Do not keep references to it outside of MemoryRepository methods
         */
        struct PerformedByWithLastSeenUpdateGuard final
        {
            PerformedByWithLastSeenUpdateGuard(const MemoryRepository& repository);
            ~PerformedByWithLastSeenUpdateGuard();

            /**
             * Get the current user that performs the action and optionally schedule the update of last seen
             */
            PerformedByType get(const Forum::Entities::EntityCollection& collection);

            /**
             * Get the current user that performs the action and optionally also perform the update of last seen
             * This method takes advantage if a write lock on the collection is already secured
             */
            Forum::Entities::UserRef getAndUpdate(Forum::Entities::EntityCollection& collection);

        private:
            MemoryRepository& repository_;
            std::function<void()> lastSeenUpdate_;
        };

        PerformedByWithLastSeenUpdateGuard preparePerformedBy(const MemoryRepository& repository);
    }
}
