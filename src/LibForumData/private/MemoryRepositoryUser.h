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
            explicit MemoryRepositoryUser(MemoryStoreRef store, Authorization::UserAuthorizationRef authorization,
                                          AuthorizationRepositoryRef authorizationRepository);

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
            StatusCode changeUserTitle(Entities::IdTypeRef id, StringView newTitle, OutStream& output) override;
            StatusCode changeUserTitle(Entities::EntityCollection& collection, Entities::IdTypeRef id, StringView newTitle) override;
            StatusCode deleteUser(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode deleteUser(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;

        private:
            StatusWithResource<Entities::UserPtr> addNewUser(Entities::EntityCollection& collection,
                                                             Entities::IdTypeRef id, Entities::User::NameType&& name,
                                                             StringView auth);
            StatusCode changeUserName(Entities::EntityCollection& collection, Entities::IdTypeRef id,
                                      Entities::User::NameType&& newName);

            Authorization::UserAuthorizationRef authorization_;
            AuthorizationRepositoryRef authorizationRepository_;
        };
    }
}
