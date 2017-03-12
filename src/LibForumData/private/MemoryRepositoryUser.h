#pragma once

#include "MemoryRepositoryCommon.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryUser final : public MemoryRepositoryBase, public IUserRepository
        {
        public:
            explicit MemoryRepositoryUser(MemoryStoreRef store);

            StatusCode getUsers(std::ostream& output, RetrieveUsersBy by) const override;

            StatusCode getUserById(const Entities::IdType& id, std::ostream& output) const override;
            StatusCode getUserByName(const std::string& name, std::ostream& output) const override;

            StatusCode addNewUser(const std::string& name, std::ostream& output) override;
            StatusCode changeUserName(const Entities::IdType& id, const std::string& newName, 
                                      std::ostream& output) override;
            StatusCode changeUserInfo(const Entities::IdType& id, const std::string& newInfo, 
                                      std::ostream& output) override;
            StatusCode deleteUser(const Entities::IdType& id, std::ostream& output) override;

        private:
            boost::u32regex validUserNameRegex;
        };
    }
}
