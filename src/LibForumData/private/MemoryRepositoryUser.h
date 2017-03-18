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

            StatusCode getUsers(OutStream& output, RetrieveUsersBy by) const override;

            StatusCode getUserById(const Entities::IdType& id, OutStream& output) const override;
            StatusCode getUserByName(const StringView& name, OutStream& output) const override;

            StatusCode addNewUser(const StringView& name, OutStream& output) override;
            StatusCode changeUserName(const Entities::IdType& id, const StringView& newName, 
                                      OutStream& output) override;
            StatusCode changeUserInfo(const Entities::IdType& id, const StringView& newInfo, 
                                      OutStream& output) override;
            StatusCode deleteUser(const Entities::IdType& id, OutStream& output) override;

        private:
            boost::u32regex validUserNameRegex;
        };
    }
}
