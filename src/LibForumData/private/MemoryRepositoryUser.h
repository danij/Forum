#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryUser final : public MemoryRepositoryBase,
                                           public IUserRepository, public IUserDirectWriteRepository
        {
        public:
            explicit MemoryRepositoryUser(MemoryStoreRef store, Authorization::UserAuthorizationRef authorization);

            StatusCode getUsers(OutStream& output, RetrieveUsersBy by) const override;

            StatusCode getUserById(Entities::IdTypeRef id, OutStream& output) const override;
            StatusCode getUserByName(StringView name, OutStream& output) const override;

            StatusCode addNewUser(StringView name, StringView auth, OutStream& output) override;
            StatusWithResource<Entities::UserPtr> addNewUser(Entities::EntityCollection& collection, 
                                                             Entities::IdTypeRef id, StringView name, StringView auth) override;
            StatusCode changeUserName(Entities::IdTypeRef id, StringView newName,  OutStream& output) override;
            StatusCode changeUserName(Entities::EntityCollection& collection, Entities::IdTypeRef id, StringView newName) override;
            StatusCode changeUserInfo(Entities::IdTypeRef id, StringView newInfo, OutStream& output) override;
            StatusCode changeUserInfo(Entities::EntityCollection& collection, Entities::IdTypeRef id, StringView newInfo) override;
            StatusCode deleteUser(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode deleteUser(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;

            StatusCode getCurrentUserPrivileges(OutStream& output) const override;
            StatusCode getRequiredPrivileges(OutStream& output) const override;
            StatusCode getDefaultPrivilegeDurations(OutStream& output) const override;
            StatusCode getAssignedPrivileges(OutStream& output) const override;

        private:
            Authorization::UserAuthorizationRef authorization_;
        };
    }
}
