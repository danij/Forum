#pragma once

#include <boost/core/noncopyable.hpp>

#include "Entities.h"
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

            virtual void getUserCount(std::ostream& output) const override;
            virtual void getUsersByName(std::ostream& output) const override;
            virtual void getUsersByCreationDate(std::ostream& output) const override;
            virtual void getUserByName(const std::string& name, std::ostream& output) const override;

            virtual void addNewUser(const std::string& name, std::ostream& output) override;
            virtual void changeUserName(const Forum::Entities::IdType& id, const std::string& newName,
                                        std::ostream& output) override;
            virtual void deleteUser(const Forum::Entities::IdType& id, std::ostream& output) override;

        private:
            PerformedByType getPerformedBy() const;

            Forum::Helpers::ResourceGuard<Forum::Entities::EntityCollection> collection_;
            mutable ObserverCollection observers_;
        };
    }
}
