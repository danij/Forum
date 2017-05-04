#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryUser final : public MemoryRepositoryBase, public IUserRepository
        {
        public:
            explicit MemoryRepositoryUser(MemoryStoreRef store, Authorization::UserAuthorizationRef authorization);

            StatusCode getUsers(OutStream& output, RetrieveUsersBy by) const override;

            StatusCode getUserById(const Entities::IdType& id, OutStream& output) const override;
            StatusCode getUserByName(StringView name, OutStream& output) const override;

            StatusCode addNewUser(StringView name, StringView auth, OutStream& output) override;
            StatusCode changeUserName(const Entities::IdType& id, StringView newName, 
                                      OutStream& output) override;
            StatusCode changeUserInfo(const Entities::IdType& id, StringView newInfo, 
                                      OutStream& output) override;
            StatusCode deleteUser(const Entities::IdType& id, OutStream& output) override;

        private:
            Authorization::UserAuthorizationRef authorization_;
        };
    }
}
