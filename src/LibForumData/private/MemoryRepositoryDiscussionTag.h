#pragma once

#include "MemoryRepositoryCommon.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionTag final : public MemoryRepositoryBase, public IDiscussionTagRepository
        {
        public:
            explicit MemoryRepositoryDiscussionTag(MemoryStoreRef store);

            StatusCode getDiscussionTags(OutStream& output, RetrieveDiscussionTagsBy by) const override;
            
            StatusCode addNewDiscussionTag(const std::string& name, OutStream& output) override;
            StatusCode changeDiscussionTagName(const Entities::IdType& id, const std::string& newName,
                                               OutStream& output) override;
            StatusCode changeDiscussionTagUiBlob(const Entities::IdType& id, const std::string& blob,
                                                 OutStream& output) override;
            StatusCode deleteDiscussionTag(const Entities::IdType& id, OutStream& output) override;
            
            StatusCode addDiscussionTagToThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                OutStream& output) override;
            StatusCode removeDiscussionTagFromThread(const Entities::IdType& tagId, 
                                                     const Entities::IdType& threadId, 
                                                     OutStream& output) override;
            StatusCode mergeDiscussionTags(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                           OutStream& output) override;
        private:
            boost::u32regex validDiscussionTagNameRegex;
        };
    }
}
