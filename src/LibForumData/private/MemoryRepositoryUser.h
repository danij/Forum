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
            StatusCode getAssignedPrivileges(Entities::IdTypeRef id, OutStream& output) const override;

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

            StatusCode changeDiscussionThreadMessageDefaultPrivilegeDuration(
                    Authorization::DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    Authorization::PrivilegeDefaultDurationIntType value, OutStream& output) override;
            StatusCode changeDiscussionThreadMessageDefaultPrivilegeDuration(
                    Entities::EntityCollection& collection,
                    Authorization::DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    Authorization::PrivilegeDefaultDurationIntType value) override;
            StatusCode changeForumWideMessageDefaultPrivilegeDuration(
                    Authorization::ForumWideDefaultPrivilegeDuration privilege,
                    Authorization::PrivilegeDefaultDurationIntType value, OutStream& output) override;
            StatusCode changeForumWideMessageDefaultPrivilegeDuration(
                    Entities::EntityCollection& collection,
                    Authorization::ForumWideDefaultPrivilegeDuration privilege,
                    Authorization::PrivilegeDefaultDurationIntType value) override;

            StatusCode assignDiscussionThreadMessagePrivilege(
                    Entities::IdTypeRef userId, Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionThreadMessagePrivilege(
                    Entities::EntityCollection& collection, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;
            StatusCode assignDiscussionThreadPrivilege(
                    Entities::IdTypeRef userId, Authorization::DiscussionThreadPrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionThreadPrivilege(
                    Entities::EntityCollection& collection, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadPrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;
            StatusCode assignDiscussionTagPrivilege(
                    Entities::IdTypeRef userId, Authorization::DiscussionTagPrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionTagPrivilege(
                    Entities::EntityCollection& collection, Entities::IdTypeRef userId,
                    Authorization::DiscussionTagPrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;
            StatusCode assignDiscussionCategoryPrivilege(
                    Entities::IdTypeRef userId, Authorization::DiscussionCategoryPrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionCategoryPrivilege(
                    Entities::EntityCollection& collection, Entities::IdTypeRef userId,
                    Authorization::DiscussionCategoryPrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;
            StatusCode assignForumWidePrivilege(
                    Entities::IdTypeRef userId, Authorization::ForumWidePrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignForumWidePrivilege(
                    Entities::EntityCollection& collection, Entities::IdTypeRef userId,
                    Authorization::ForumWidePrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;




        private:
            Authorization::UserAuthorizationRef authorization_;
        };
    }
}
