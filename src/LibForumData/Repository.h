#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

#include "Entities.h"
#include "Observers.h"
#include "TypeHelpers.h"

namespace Forum
{
    namespace Repository
    {
        class IReadRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IReadRepository);

            virtual void addObserver(const ReadRepositoryObserverRef& observer) = 0;
            virtual void removeObserver(const ReadRepositoryObserverRef& observer) = 0;

            virtual void getEntitiesCount(std::ostream& output) const = 0;

            virtual void getUsersByName(std::ostream& output) const = 0;
            virtual void getUsersByCreatedAscending(std::ostream& output) const = 0;
            virtual void getUsersByCreatedDescending(std::ostream& output) const = 0;
            virtual void getUsersByLastSeenAscending(std::ostream& output) const = 0;
            virtual void getUsersByLastSeenDescending(std::ostream& output) const = 0;
            virtual void getUserById(const Forum::Entities::IdType& id, std::ostream& output) const = 0;
            virtual void getUserByName(const std::string& name, std::ostream& output) const = 0;

            virtual void getDiscussionThreadsByName(std::ostream& output) const = 0;
            virtual void getDiscussionThreadsByCreatedAscending(std::ostream& output) const = 0;
            virtual void getDiscussionThreadsByCreatedDescending(std::ostream& output) const = 0;
            virtual void getDiscussionThreadsByLastUpdatedAscending(std::ostream& output) const = 0;
            virtual void getDiscussionThreadsByLastUpdatedDescending(std::ostream& output) const = 0;
            virtual void getDiscussionThreadById(const Forum::Entities::IdType& id, std::ostream& output) const = 0;

            virtual void getDiscussionThreadsOfUserByName(const Forum::Entities::IdType& id,
                                                          std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfUserByCreatedAscending(const Forum::Entities::IdType& id,
                                                                      std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfUserByCreatedDescending(const Forum::Entities::IdType& id,
                                                                       std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfUserByLastUpdatedAscending(const Forum::Entities::IdType& id,
                                                                          std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfUserByLastUpdatedDescending(const Forum::Entities::IdType& id,
                                                                           std::ostream& output) const = 0;

            virtual void getDiscussionThreadMessagesOfUserByCreatedAscending(const Forum::Entities::IdType& id,
                                                                             std::ostream& output) const = 0;
            virtual void getDiscussionThreadMessagesOfUserByCreatedDescending(const Forum::Entities::IdType& id,
                                                                              std::ostream& output) const = 0;
        };

        typedef std::shared_ptr<IReadRepository> ReadRepositoryRef;

        enum StatusCode : uint32_t
        {
            OK = 0,
            INVALID_PARAMETERS,
            VALUE_TOO_LONG,
            VALUE_TOO_SHORT,
            ALREADY_EXISTS,
            NOT_FOUND
        };

        class IWriteRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IWriteRepository);

            virtual void addObserver(const WriteRepositoryObserverRef& observer) = 0;
            virtual void removeObserver(const WriteRepositoryObserverRef& observer) = 0;

            virtual void addNewUser(const std::string& name, std::ostream& output) = 0;
            virtual void changeUserName(const Forum::Entities::IdType& id, const std::string& newName,
                                        std::ostream& output) = 0;
            virtual void deleteUser(const Forum::Entities::IdType& id, std::ostream& output) = 0;

            virtual void addNewDiscussionThread(const std::string& name, std::ostream& output) = 0;
            virtual void changeDiscussionThreadName(const Forum::Entities::IdType& id, const std::string& newName,
                                                    std::ostream& output) = 0;
            virtual void deleteDiscussionThread(const Forum::Entities::IdType& id, std::ostream& output) = 0;

            virtual void addNewDiscussionMessageInThread(const Forum::Entities::IdType& threadId,
                                                         const std::string& content, std::ostream& output) = 0;
            virtual void deleteDiscussionMessage(const Forum::Entities::IdType& id, std::ostream& output) = 0;
        };

        typedef std::shared_ptr<IWriteRepository> WriteRepositoryRef;


        class IMetricsRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IMetricsRepository);

            virtual void getVersion(std::ostream& output) = 0;
        };

        typedef std::shared_ptr<IMetricsRepository> MetricsRepositoryRef;
    }
}
