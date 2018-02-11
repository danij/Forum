/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
            StatusCode getUsersOnline(OutStream& output) const override;

            StatusCode getUserById(Entities::IdTypeRef id, OutStream& output) const override;
            StatusCode getUserByName(StringView name, OutStream& output) const override;
            StatusCode searchUsersByName(StringView name, OutStream& output) const override;
            StatusCode getUserLogo(Entities::IdTypeRef id, OutStream& output) const override;
            StatusCode getUserVoteHistory(Entities::IdTypeRef id, OutStream& output) const override;

            StatusCode addNewUser(StringView name, StringView auth, OutStream& output) override;
            StatusWithResource<Entities::UserPtr> addNewUser(Entities::EntityCollection& collection,
                                                             Entities::IdTypeRef id, StringView name, StringView auth) override;
            StatusCode changeUserName(Entities::IdTypeRef id, StringView newName,  OutStream& output) override;
            StatusCode changeUserName(Entities::EntityCollection& collection, Entities::IdTypeRef id, StringView newName) override;
            StatusCode changeUserInfo(Entities::IdTypeRef id, StringView newInfo, OutStream& output) override;
            StatusCode changeUserInfo(Entities::EntityCollection& collection, Entities::IdTypeRef id, StringView newInfo) override;
            StatusCode changeUserTitle(Entities::IdTypeRef id, StringView newTitle, OutStream& output) override;
            StatusCode changeUserTitle(Entities::EntityCollection& collection, Entities::IdTypeRef id, StringView newTitle) override;
            StatusCode changeUserSignature(Entities::IdTypeRef id, StringView newSignature, OutStream& output) override;
            StatusCode changeUserSignature(Entities::EntityCollection& collection, Entities::IdTypeRef id,
                                           StringView newSignature) override;
            StatusCode changeUserLogo(Entities::IdTypeRef id, StringView newLogo, OutStream& output) override;
            StatusCode changeUserLogo(Entities::EntityCollection& collection, Entities::IdTypeRef id,
                                      StringView newLogo) override;
            StatusCode deleteUserLogo(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode deleteUserLogo(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;
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
