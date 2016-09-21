#pragma once

#include <boost/core/noncopyable.hpp>

#include "Entities.h"
#include "Repository.h"
#include "ResourceGuard.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepository final : public IReadRepository, private boost::noncopyable
        {
        public:
            MemoryRepository();

            virtual void getUserCount(std::ostream& output) const override;

        private:
            Forum::Helpers::ResourceGuard<Forum::Entities::EntityCollection> collection_;
        };
    }
}
