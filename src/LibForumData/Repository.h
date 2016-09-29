#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

#include "Entities.h"

namespace Forum
{
    namespace Repository
    {
        class IReadRepository
        {
        public:
            virtual ~IReadRepository() = default;

            virtual void getUserCount(std::ostream& output) const = 0;
            virtual void getUsers(std::ostream& output) const = 0;
            virtual void getUserByName(const std::string& name, std::ostream& output) const = 0;
        };

        typedef std::shared_ptr<const IReadRepository> ReadRepositoryConstRef;

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
            virtual ~IWriteRepository() = default;

            virtual void addNewUser(const std::string& name, std::ostream& output) = 0;
            virtual void changeUserName(const Forum::Entities::IdType& id, const std::string& newName,
                                        std::ostream& output) = 0;
            virtual void deleteUser(const Forum::Entities::IdType& id, std::ostream& output) = 0;
        };

        typedef std::shared_ptr<IWriteRepository> WriteRepositoryRef;


        class IMetricsRepository
        {
        public:
            virtual ~IMetricsRepository() = default;

            virtual void getVersion(std::ostream& output) = 0;
        };

        typedef std::shared_ptr<IMetricsRepository> MetricsRepositoryRef;
    }
}
