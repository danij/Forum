#pragma once

#include <iosfwd>

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
    }
}
