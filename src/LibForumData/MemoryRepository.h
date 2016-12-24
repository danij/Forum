#pragma once

#include "ContextProviders.h"
#include "EntityCollection.h"
#include "ObserverCollection.h"
#include "Repository.h"
#include "ResourceGuard.h"

#include <boost/core/noncopyable.hpp>

namespace Forum
{
    namespace Repository
    {
        class MemoryRepository;

        /**
        * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
        * The update is performed on the spot if a write lock is held or
        * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
        * Do not keep references to it outside of MemoryRepository methods
        */
        struct PerformedByWithLastSeenUpdateGuard final
        {
            explicit PerformedByWithLastSeenUpdateGuard(const MemoryRepository& repository);
            ~PerformedByWithLastSeenUpdateGuard();

            /**
            * Get the current user that performs the action and optionally schedule the update of last seen
            */
            PerformedByType get(const Entities::EntityCollection& collection);

            /**
            * Get the current user that performs the action and optionally also perform the update of last seen
            * This method takes advantage if a write lock on the collection is already secured
            */
            Entities::UserRef getAndUpdate(Entities::EntityCollection& collection);

        private:
            MemoryRepository& repository_;
            std::function<void()> lastSeenUpdate_;
        };

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
            virtual void getUsersByCreatedAscending(std::ostream& output) const override;
            virtual void getUsersByCreatedDescending(std::ostream& output) const override;
            virtual void getUsersByLastSeenAscending(std::ostream& output) const override;
            virtual void getUsersByLastSeenDescending(std::ostream& output) const override;

            virtual void getUserById(const Entities::IdType& id, std::ostream& output) const override;
            virtual void getUserByName(const std::string& name, std::ostream& output) const override;

            virtual void addNewUser(const std::string& name, std::ostream& output) override;
            virtual void changeUserName(const Entities::IdType& id, const std::string& newName,
                                        std::ostream& output) override;
            virtual void deleteUser(const Entities::IdType& id, std::ostream& output) override;

            virtual void getDiscussionThreadsByName(std::ostream& output) const override;
            virtual void getDiscussionThreadsByCreatedAscending(std::ostream& output) const override;
            virtual void getDiscussionThreadsByCreatedDescending(std::ostream& output) const override;
            virtual void getDiscussionThreadsByLastUpdatedAscending(std::ostream& output) const override;
            virtual void getDiscussionThreadsByLastUpdatedDescending(std::ostream& output) const override;
            virtual void getDiscussionThreadsByMessageCountAscending(std::ostream& output) const override;
            virtual void getDiscussionThreadsByMessageCountDescending(std::ostream& output) const override;
            virtual void getDiscussionThreadById(const Entities::IdType& id, std::ostream& output) const override;

            virtual void getDiscussionThreadsOfUserByName(const Entities::IdType& id,
                                                          std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByCreatedAscending(const Entities::IdType& id,
                                                                      std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByCreatedDescending(const Entities::IdType& id,
                                                                       std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByLastUpdatedAscending(const Entities::IdType& id,
                                                                          std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByLastUpdatedDescending(const Entities::IdType& id,
                                                                           std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByMessageCountAscending(const Entities::IdType& id,
                                                                           std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByMessageCountDescending(const Entities::IdType& id,
                                                                            std::ostream& output) const override;

            virtual void addNewDiscussionThread(const std::string& name, std::ostream& output) override;
            virtual void changeDiscussionThreadName(const Entities::IdType& id, const std::string& newName,
                                                    std::ostream& output) override;
            virtual void deleteDiscussionThread(const Entities::IdType& id, std::ostream& output) override;

            virtual void addNewDiscussionMessageInThread(const Entities::IdType& threadId,
                                                         const std::string& content, std::ostream& output) override;
            virtual void deleteDiscussionMessage(const Entities::IdType& id, std::ostream& output) override;

            virtual void getDiscussionThreadMessagesOfUserByCreatedAscending(const Entities::IdType& id,
                                                                             std::ostream& output) const override;
            virtual void getDiscussionThreadMessagesOfUserByCreatedDescending(const Entities::IdType& id,
                                                                              std::ostream& output) const override;

        private:
            friend struct PerformedByWithLastSeenUpdateGuard;

            PerformedByWithLastSeenUpdateGuard preparePerformedBy() const
            {
                return PerformedByWithLastSeenUpdateGuard(*this);
            }

            void getUsersByCreated(bool ascending, std::ostream& output) const;
            void getUsersByLastSeen(bool ascending, std::ostream& output) const;

            void getDiscussionThreadsByCreated(bool ascending, std::ostream& output) const;
            void getDiscussionThreadsByLastUpdated(bool ascending, std::ostream& output) const;
            void getDiscussionThreadsByMessageCount(bool ascending, std::ostream& output) const;

            void getDiscussionThreadsOfUserByCreated(bool ascending, const Entities::IdType& id,
                                                     std::ostream& output) const;
            void getDiscussionThreadsOfUserByLastUpdated(bool ascending, const Entities::IdType& id,
                                                         std::ostream& output) const;
            void getDiscussionThreadsOfUserByMessageCount(bool ascending, const Entities::IdType& id,
                                                          std::ostream& output) const;

            void getDiscussionThreadMessagesOfUserByCreated(bool ascending, const Entities::IdType& id,
                                                            std::ostream& output) const;

            Helpers::ResourceGuard<Entities::EntityCollection> collection_;
            mutable ObserverCollection observers_;
        };

        inline ObserverContext_ createObserverContext(PerformedByType performedBy)
        {
            return ObserverContext_(performedBy, Context::getCurrentTime());
        }
    }
}
