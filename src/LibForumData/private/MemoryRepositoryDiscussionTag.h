#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionTag final : public MemoryRepositoryBase, public IDiscussionTagRepository
        {
        public:
            explicit MemoryRepositoryDiscussionTag(MemoryStoreRef store,
                                                   Authorization::DiscussionTagAuthorizationRef authorization);

            StatusCode getDiscussionTags(OutStream& output, RetrieveDiscussionTagsBy by) const override;
            
            StatusCode addNewDiscussionTag(StringView name, OutStream& output) override;
            StatusCode changeDiscussionTagName(Entities::IdTypeRef id, StringView newName,
                                               OutStream& output) override;
            StatusCode changeDiscussionTagUiBlob(Entities::IdTypeRef id, StringView blob,
                                                 OutStream& output) override;
            StatusCode deleteDiscussionTag(Entities::IdTypeRef id, OutStream& output) override;
            
            StatusCode addDiscussionTagToThread(Entities::IdTypeRef tagId, Entities::IdTypeRef threadId, 
                                                OutStream& output) override;
            StatusCode removeDiscussionTagFromThread(Entities::IdTypeRef tagId, 
                                                     Entities::IdTypeRef threadId, 
                                                     OutStream& output) override;
            StatusCode mergeDiscussionTags(Entities::IdTypeRef fromId, Entities::IdTypeRef intoId,
                                           OutStream& output) override;
        private:
            Authorization::DiscussionTagAuthorizationRef authorization_;
        };
    }
}
