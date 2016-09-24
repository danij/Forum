#pragma once

#include <cstdint>
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
            virtual void getUsers(std::ostream& output) const = 0;
        };

        typedef std::shared_ptr<const IReadRepository> ReadRepositoryConstRef;

        enum StatusCode : uint32_t
        {
            OK = 0,
            INVALID_PARAMETERS,
            VALUE_TOO_LONG,
            ALREADY_EXISTS,
            OUTPUT_ALREADY_WRITTEN = 0x10000000
        };

        inline bool statusCodeNotWritten(StatusCode value) { return (value & StatusCode::OUTPUT_ALREADY_WRITTEN) == 0; }

        class IWriteRepository
        {
        public:
            virtual ~IWriteRepository() = default;

            virtual StatusCode addNewUser(const std::string& name, std::ostream& output) = 0;
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
