#pragma once

#include <iosfwd>
#include <memory>
#include <string>

namespace Forum
{
    namespace Repository
    {
        class IReadRepository
        {
        public:
            virtual ~IReadRepository() = default;

            virtual void getUserCount(std::ostream& output) const = 0;
        };

        typedef std::shared_ptr<const IReadRepository> ReadRepositoryConstRef;

        enum class StatusCode
        {
            OUTPUT_ALREADY_WRITTEN = -1,
            OK = 0,
            INVALID_PARAMETERS,
        };

        class IWriteRepository
        {
        public:
            virtual ~IWriteRepository() = default;

            virtual StatusCode addNewUser(const std::string& name, std::ostream& output) = 0;
        };

        typedef std::shared_ptr<IWriteRepository> WriteRepositoryRef;
    }
}
