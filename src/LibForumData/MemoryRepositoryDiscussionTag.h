#pragma once

#include "MemoryRepositoryCommon.h"

#include <boost/regex/icu.hpp>

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionTag final : public MemoryRepositoryBase, public IDiscussionTagRepository
        {
        public:
            explicit MemoryRepositoryDiscussionTag(MemoryStoreRef store);

            StatusCode getDiscussionTags(std::ostream& output, RetrieveDiscussionTagsBy by) const override;
            
            StatusCode addNewDiscussionTag(const std::string& name, std::ostream& output) override;
            StatusCode changeDiscussionTagName(const Entities::IdType& id, const std::string& newName,
                                               std::ostream& output) override;
            StatusCode changeDiscussionTagUiBlob(const Entities::IdType& id, const std::string& blob,
                                                 std::ostream& output) override;
            StatusCode deleteDiscussionTag(const Entities::IdType& id, std::ostream& output) override;
            
            StatusCode addDiscussionTagToThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                std::ostream& output) override;
            StatusCode removeDiscussionTagFromThread(const Entities::IdType& tagId, 
                                                     const Entities::IdType& threadId, 
                                                     std::ostream& output) override;
            StatusCode mergeDiscussionTags(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                           std::ostream& output) override;
        private:
            boost::u32regex validDiscussionTagNameRegex;
        };
    }
}
