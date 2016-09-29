#pragma once

#include <boost/core/noncopyable.hpp>

#include "Entities.h"
#include "EntityCollection.h"
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

            virtual void getUserCount(std::ostream& output) const override;
            virtual void getUsers(std::ostream& output) const override;
            virtual void getUserByName(const std::string& name, std::ostream& output) const override;

            virtual void addNewUser(const std::string& name, std::ostream& output) override;
            virtual void changeUserName(const Forum::Entities::IdType& id, const std::string& newName,
                                        std::ostream& output) override;

        private:
            Forum::Helpers::ResourceGuard<Forum::Entities::EntityCollection> collection_;
        };
    }
}
