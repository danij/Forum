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
#include "AuthorizationGrantedPrivilegeStore.h"
#include "JsonWriter.h"

namespace Forum::Repository
{
    class MemoryRepositoryAuthorization final : public MemoryRepositoryBase,
                                                public IAuthorizationRepository,
                                                public IAuthorizationDirectWriteRepository
    {
    public:
        explicit MemoryRepositoryAuthorization(MemoryStoreRef store,
                                               Authorization::ForumWideAuthorizationRef forumWideAuthorization,
                                               Authorization::DiscussionThreadAuthorizationRef threadAuthorization,
                                               Authorization::DiscussionThreadMessageAuthorizationRef threadMessageAuthorization,
                                               Authorization::DiscussionTagAuthorizationRef tagAuthorization,
                                               Authorization::DiscussionCategoryAuthorizationRef categoryAuthorization);
        //
        //discussion thread message
        //
        StatusCode getRequiredPrivilegesForThreadMessage(Entities::IdTypeRef messageId, OutStream& output) const override;
        StatusCode getAssignedPrivilegesForThreadMessage(Entities::IdTypeRef messageId, OutStream& output) const override;

        StatusCode changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(
                Entities::IdTypeRef messageId, Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(
                Entities::EntityCollection& collection,
                Entities::IdTypeRef messageId, Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;

        StatusCode assignDiscussionThreadMessagePrivilege(
                Entities::IdTypeRef messageId, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDurationIntType duration,
                OutStream& output) override;
        StatusCode assignDiscussionThreadMessagePrivilege(
                Entities::EntityCollection& collection,Entities::IdTypeRef messageId, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value,
                Authorization::PrivilegeDurationIntType duration) override;
        //
        //discussion thread
        //
        StatusCode getRequiredPrivilegesForThread(Entities::IdTypeRef threadId, OutStream& output) const override;
        StatusCode getAssignedPrivilegesForThread(Entities::IdTypeRef threadId, OutStream& output) const override;

        StatusCode changeDiscussionThreadMessageRequiredPrivilegeForThread(
                Entities::IdTypeRef threadId, Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionThreadMessageRequiredPrivilegeForThread(
                Entities::EntityCollection& collection,
                Entities::IdTypeRef threadId, Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;
        StatusCode changeDiscussionThreadRequiredPrivilegeForThread(
                Entities::IdTypeRef threadId, Authorization::DiscussionThreadPrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionThreadRequiredPrivilegeForThread(
                Entities::EntityCollection& collection,
                Entities::IdTypeRef threadId, Authorization::DiscussionThreadPrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;

        StatusCode assignDiscussionThreadPrivilege(
                Entities::IdTypeRef threadId, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDurationIntType duration,
                OutStream& output) override;
        StatusCode assignDiscussionThreadPrivilege(
                Entities::EntityCollection& collection, Entities::IdTypeRef threadId, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value,
                Authorization::PrivilegeDurationIntType duration) override;
        //
        //discussion tag
        //
        StatusCode getRequiredPrivilegesForTag(Entities::IdTypeRef tagId, OutStream& output) const override;
        StatusCode getAssignedPrivilegesForTag(Entities::IdTypeRef tagId, OutStream& output) const override;

        StatusCode changeDiscussionThreadMessageRequiredPrivilegeForTag(
                Entities::IdTypeRef tagId, Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionThreadMessageRequiredPrivilegeForTag(
                Entities::EntityCollection& collection,
                Entities::IdTypeRef tagId, Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;
        StatusCode changeDiscussionThreadRequiredPrivilegeForTag(
                Entities::IdTypeRef tagId, Authorization::DiscussionThreadPrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionThreadRequiredPrivilegeForTag(
                Entities::EntityCollection& collection,
                Entities::IdTypeRef tagId, Authorization::DiscussionThreadPrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;
        StatusCode changeDiscussionTagRequiredPrivilegeForTag(
                Entities::IdTypeRef tagId, Authorization::DiscussionTagPrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionTagRequiredPrivilegeForTag(
                Entities::EntityCollection& collection,
                Entities::IdTypeRef tagId, Authorization::DiscussionTagPrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;

        StatusCode assignDiscussionTagPrivilege(
                Entities::IdTypeRef tagId, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDurationIntType duration,
                OutStream& output) override;
        StatusCode assignDiscussionTagPrivilege(
                Entities::EntityCollection& collection, Entities::IdTypeRef tagId, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value,
                Authorization::PrivilegeDurationIntType duration) override;
        //
        //discussion category
        //
        StatusCode getRequiredPrivilegesForCategory(Entities::IdTypeRef categoryId, OutStream& output) const override;
        StatusCode getAssignedPrivilegesForCategory(Entities::IdTypeRef categoryId, OutStream& output) const override;

        StatusCode changeDiscussionCategoryRequiredPrivilegeForCategory(
                Entities::IdTypeRef categoryId, Authorization::DiscussionCategoryPrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionCategoryRequiredPrivilegeForCategory(
                Entities::EntityCollection& collection,
                Entities::IdTypeRef categoryId, Authorization::DiscussionCategoryPrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;

        StatusCode assignDiscussionCategoryPrivilege(
                Entities::IdTypeRef categoryId, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDurationIntType duration,
                OutStream& output) override;
        StatusCode assignDiscussionCategoryPrivilege(
                Entities::EntityCollection& collection, Entities::IdTypeRef categoryId, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value,
                Authorization::PrivilegeDurationIntType duration) override;
        //
        //forum wide
        //
        StatusCode getForumWideCurrentUserPrivileges(OutStream& output) const override;
        StatusCode getForumWideRequiredPrivileges(OutStream& output) const override;
        StatusCode getForumWideDefaultPrivilegeLevels(OutStream& output) const override;
        StatusCode getForumWideAssignedPrivileges(OutStream& output) const override;
        StatusCode getAssignedPrivilegesForUser(Entities::IdTypeRef userId, OutStream& output) const override;

        StatusCode changeDiscussionThreadMessageRequiredPrivilege(
                Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionThreadMessageRequiredPrivilege(
                Entities::EntityCollection& collection, Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;
        StatusCode changeDiscussionThreadRequiredPrivilege(
                Authorization::DiscussionThreadPrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionThreadRequiredPrivilege(
                Entities::EntityCollection& collection, Authorization::DiscussionThreadPrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;
        StatusCode changeDiscussionTagRequiredPrivilege(
                Authorization::DiscussionTagPrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionTagRequiredPrivilege(
                Entities::EntityCollection& collection, Authorization::DiscussionTagPrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;
        StatusCode changeDiscussionCategoryRequiredPrivilege(
                Authorization::DiscussionCategoryPrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeDiscussionCategoryRequiredPrivilege(
                Entities::EntityCollection& collection, Authorization::DiscussionCategoryPrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;
        StatusCode changeForumWideRequiredPrivilege(
                Authorization::ForumWidePrivilege privilege,
                Authorization::PrivilegeValueIntType value, OutStream& output) override;
        StatusCode changeForumWideRequiredPrivilege(
                Entities::EntityCollection& collection, Authorization::ForumWidePrivilege privilege,
                Authorization::PrivilegeValueIntType value) override;

        StatusCode changeForumWideDefaultPrivilegeLevel(
                Authorization::ForumWideDefaultPrivilegeDuration privilege,
                Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDurationIntType duration,
                OutStream& output) override;
        StatusCode changeForumWideDefaultPrivilegeLevel(
                Entities::EntityCollection& collection,
                Authorization::ForumWideDefaultPrivilegeDuration privilege,
            Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDurationIntType duration) override;

        StatusCode assignForumWidePrivilege(
                Entities::IdTypeRef userId, Authorization::PrivilegeValueIntType value,
                Authorization::PrivilegeDurationIntType duration, OutStream& output) override;
        StatusCode assignForumWidePrivilege(
                Entities::EntityCollection& collection, Entities::IdTypeRef userId,
                Authorization::PrivilegeValueIntType value,
                Authorization::PrivilegeDurationIntType duration) override;
    private:
        static void writeDiscussionThreadMessageRequiredPrivileges(
                const Authorization::DiscussionThreadMessagePrivilegeStore& store, Json::JsonWriter& writer);
        static void writeDiscussionThreadRequiredPrivileges(
                const Authorization::DiscussionThreadPrivilegeStore& store, Json::JsonWriter& writer);
        static void writeDiscussionTagRequiredPrivileges(
                const Authorization::DiscussionTagPrivilegeStore& store, Json::JsonWriter& writer);
        static void writeDiscussionCategoryRequiredPrivileges(
                const Authorization::DiscussionCategoryPrivilegeStore& store, Json::JsonWriter& writer);
        static void writeForumWideRequiredPrivileges(
                const Authorization::ForumWidePrivilegeStore& store, Json::JsonWriter& writer);

        static void writeForumWideDefaultPrivilegeLevels(
                const Authorization::ForumWidePrivilegeStore& store, Json::JsonWriter& writer);

        static void writeDiscussionThreadMessageAssignedPrivileges(const Entities::EntityCollection& collection,
                                                                   Entities::IdTypeRef id, Json::JsonWriter& writer);
        static void writeDiscussionThreadAssignedPrivileges(const Entities::EntityCollection& collection,
                                                            Entities::IdTypeRef id, Json::JsonWriter& writer);
        static void writeDiscussionTagAssignedPrivileges(const Entities::EntityCollection& collection,
                                                         Entities::IdTypeRef id, Json::JsonWriter& writer);
        static void writeDiscussionCategoryAssignedPrivileges(const Entities::EntityCollection& collection,
                                                              Entities::IdTypeRef id, Json::JsonWriter& writer);
        static void writeForumWideAssignedPrivileges(const Entities::EntityCollection& collection,
                                                     Entities::IdTypeRef id, Json::JsonWriter& writer);

        static void writeDiscussionThreadUserAssignedPrivileges(const Entities::EntityCollection& collection,
                                                                const Authorization::SerializationRestriction& restriction,
                                                                Entities::IdTypeRef userId,
                                                                Json::JsonWriter& writer);
        static void writeDiscussionTagUserAssignedPrivileges(const Entities::EntityCollection& collection,
                                                             const Authorization::SerializationRestriction& restriction,
                                                             Entities::IdTypeRef userId,
                                                             Json::JsonWriter& writer);
        static void writeDiscussionCategoryUserAssignedPrivileges(const Entities::EntityCollection& collection,
                                                                  const Authorization::SerializationRestriction& restriction,
                                                                  Entities::IdTypeRef userId,
                                                                  Json::JsonWriter& writer);
        static void writeForumWideUserAssignedPrivileges(const Entities::EntityCollection& collection,
                                                         const Authorization::SerializationRestriction& restriction,
                                                         Entities::IdTypeRef userId,
                                                         Json::JsonWriter& writer);

        Authorization::ForumWideAuthorizationRef forumWideAuthorization_;
        Authorization::DiscussionThreadAuthorizationRef threadAuthorization_;
        Authorization::DiscussionThreadMessageAuthorizationRef threadMessageAuthorization_;
        Authorization::DiscussionTagAuthorizationRef tagAuthorization_;
        Authorization::DiscussionCategoryAuthorizationRef categoryAuthorization_;
    };
}
