#pragma once

#include "MemoryRepositoryCommon.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionCategory final : public MemoryRepositoryBase, public IDiscussionCategoryRepository
        {
        public:
            explicit MemoryRepositoryDiscussionCategory(MemoryStoreRef store);

            StatusCode addNewDiscussionCategory(const StringView& name, const Entities::IdType& parentId,
                                                OutStream& output) override;
            StatusCode changeDiscussionCategoryName(const Entities::IdType& id, const StringView& newName,
                                                    OutStream& output) override;
            StatusCode changeDiscussionCategoryDescription(const Entities::IdType& id, 
                                                           const StringView& newDescription,
                                                           OutStream& output) override;
            StatusCode changeDiscussionCategoryParent(const Entities::IdType& id, 
                                                      const Entities::IdType& newParentId,
                                                      OutStream& output) override;
            StatusCode changeDiscussionCategoryDisplayOrder(const Entities::IdType& id, 
                                                            int_fast16_t newDisplayOrder,
                                                            OutStream& output) override;
            StatusCode deleteDiscussionCategory(const Entities::IdType& id, OutStream& output) override;

            StatusCode getDiscussionCategoryById(const Entities::IdType& id, OutStream& output) const override;
            StatusCode getDiscussionCategories(OutStream& output, RetrieveDiscussionCategoriesBy by) const override;
            StatusCode getDiscussionCategoriesFromRoot(OutStream& output) const override;

            StatusCode addDiscussionTagToCategory(const Entities::IdType& tagId, 
                                                  const Entities::IdType& categoryId, 
                                                  OutStream& output) override;
            StatusCode removeDiscussionTagFromCategory(const Entities::IdType& tagId, 
                                                       const Entities::IdType& categoryId, 
                                                       OutStream& output) override;
        private:
            boost::u32regex validDiscussionCategoryNameRegex;
        };
    }
}
