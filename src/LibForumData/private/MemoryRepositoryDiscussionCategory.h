#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionCategory final : public MemoryRepositoryBase, public IDiscussionCategoryRepository
        {
        public:
            explicit MemoryRepositoryDiscussionCategory(MemoryStoreRef store,
                                                        Authorization::DiscussionCategoryAuthorizationRef authorization);

            StatusCode addNewDiscussionCategory(StringView name, Entities::IdTypeRef parentId,
                                                OutStream& output) override;
            StatusCode changeDiscussionCategoryName(Entities::IdTypeRef id, StringView newName,
                                                    OutStream& output) override;
            StatusCode changeDiscussionCategoryDescription(Entities::IdTypeRef id, 
                                                           StringView newDescription,
                                                           OutStream& output) override;
            StatusCode changeDiscussionCategoryParent(Entities::IdTypeRef id, 
                                                      Entities::IdTypeRef newParentId,
                                                      OutStream& output) override;
            StatusCode changeDiscussionCategoryDisplayOrder(Entities::IdTypeRef id, 
                                                            int_fast16_t newDisplayOrder,
                                                            OutStream& output) override;
            StatusCode deleteDiscussionCategory(Entities::IdTypeRef id, OutStream& output) override;

            StatusCode getDiscussionCategoryById(Entities::IdTypeRef id, OutStream& output) const override;
            StatusCode getDiscussionCategories(OutStream& output, RetrieveDiscussionCategoriesBy by) const override;
            StatusCode getDiscussionCategoriesFromRoot(OutStream& output) const override;

            StatusCode addDiscussionTagToCategory(Entities::IdTypeRef tagId, 
                                                  Entities::IdTypeRef categoryId, 
                                                  OutStream& output) override;
            StatusCode removeDiscussionTagFromCategory(Entities::IdTypeRef tagId, 
                                                       Entities::IdTypeRef categoryId, 
                                                       OutStream& output) override;
        private:
            Authorization::DiscussionCategoryAuthorizationRef authorization_;
        };
    }
}
