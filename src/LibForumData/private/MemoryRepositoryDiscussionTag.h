#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionTag final : public MemoryRepositoryBase,
                                                    public IDiscussionTagRepository,
                                                    public IDiscussionTagDirectWriteRepository
        {
        public:
            explicit MemoryRepositoryDiscussionTag(MemoryStoreRef store,
                                                   Authorization::DiscussionTagAuthorizationRef authorization);

            StatusCode getDiscussionTags(OutStream& output, RetrieveDiscussionTagsBy by) const override;

            StatusCode addNewDiscussionTag(StringView name, OutStream& output) override;
            StatusWithResource<Entities::DiscussionTagPtr>
                addNewDiscussionTag(Entities::EntityCollection& collection, Entities::IdTypeRef id, StringView name) override;
            StatusCode changeDiscussionTagName(Entities::IdTypeRef id, StringView newName, OutStream& output) override;
            StatusCode changeDiscussionTagName(Entities::EntityCollection& collection, Entities::IdTypeRef id,
                                               StringView newName) override;
            StatusCode changeDiscussionTagUiBlob(Entities::IdTypeRef id, StringView blob, OutStream& output) override;
            StatusCode changeDiscussionTagUiBlob(Entities::EntityCollection& collection,
                                                 Entities::IdTypeRef id, StringView blob) override;
            StatusCode deleteDiscussionTag(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode deleteDiscussionTag(Entities::EntityCollection& collection,  Entities::IdTypeRef id) override;

            StatusCode addDiscussionTagToThread(Entities::IdTypeRef tagId, Entities::IdTypeRef threadId,
                                                OutStream& output) override;
            StatusCode addDiscussionTagToThread(Entities::EntityCollection& collection,
                                                Entities::IdTypeRef tagId, Entities::IdTypeRef threadId) override;
            StatusCode removeDiscussionTagFromThread(Entities::IdTypeRef tagId, Entities::IdTypeRef threadId,
                                                     OutStream& output) override;
            StatusCode removeDiscussionTagFromThread(Entities::EntityCollection& collection,
                                                     Entities::IdTypeRef tagId, Entities::IdTypeRef threadId) override;
            StatusCode mergeDiscussionTags(Entities::IdTypeRef fromId, Entities::IdTypeRef intoId,
                                           OutStream& output) override;
            StatusCode mergeDiscussionTags(Entities::EntityCollection& collection, Entities::IdTypeRef fromId,
                                           Entities::IdTypeRef intoId) override;

            StatusCode getRequiredPrivileges(Entities::IdTypeRef tagId, OutStream& output) const override;
            StatusCode getDefaultPrivilegeDurations(Entities::IdTypeRef tagId, OutStream& output) const override;
            StatusCode getAssignedPrivileges(Entities::IdTypeRef tagId, OutStream& output) const override;

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

            StatusCode changeDiscussionThreadMessageDefaultPrivilegeDurationForTag(
                    Entities::IdTypeRef tagId, Authorization::DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    Authorization::PrivilegeDefaultDurationIntType value, OutStream& output) override;
            StatusCode changeDiscussionThreadMessageDefaultPrivilegeDurationForTag(
                    Entities::EntityCollection& collection,
                    Entities::IdTypeRef tagId, Authorization::DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    Authorization::PrivilegeDefaultDurationIntType value) override;

            StatusCode assignDiscussionThreadMessagePrivilegeForTag(
                    Entities::IdTypeRef tagId, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionThreadMessagePrivilegeForTag(
                    Entities::EntityCollection& collection, Entities::IdTypeRef tagId, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;
            StatusCode assignDiscussionThreadPrivilegeForTag(
                    Entities::IdTypeRef tagId, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadPrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionThreadPrivilegeForTag(
                    Entities::EntityCollection& collection, Entities::IdTypeRef tagId, Entities::IdTypeRef userId,
                    Authorization::DiscussionThreadPrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;
            StatusCode assignDiscussionTagPrivilegeForTag(
                    Entities::IdTypeRef tagId, Entities::IdTypeRef userId,
                    Authorization::DiscussionTagPrivilege privilege,
                    Authorization::PrivilegeValueIntType value, Authorization::PrivilegeDefaultDurationIntType duration,
                    OutStream& output) override;
            StatusCode assignDiscussionTagPrivilegeForTag(
                    Entities::EntityCollection& collection, Entities::IdTypeRef tagId, Entities::IdTypeRef userId,
                    Authorization::DiscussionTagPrivilege privilege,
                    Authorization::PrivilegeValueIntType value,
                    Authorization::PrivilegeDefaultDurationIntType duration) override;
        private:
            Authorization::DiscussionTagAuthorizationRef authorization_;
        };
    }
}
