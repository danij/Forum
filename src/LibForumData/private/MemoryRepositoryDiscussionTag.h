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
        private:
            StatusWithResource<Entities::DiscussionTagPtr>
                    addNewDiscussionTag(Entities::EntityCollection& collection, Entities::IdTypeRef id,
                                        Entities::DiscussionTag::NameType&& name);


            Authorization::DiscussionTagAuthorizationRef authorization_;
        };
    }
}
