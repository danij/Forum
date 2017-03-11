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

            StatusCode addNewDiscussionCategory(const std::string& name, const Entities::IdType& parentId,
                                                std::ostream& output) override;
            StatusCode changeDiscussionCategoryName(const Entities::IdType& id, const std::string& newName,
                                                    std::ostream& output) override;
            StatusCode changeDiscussionCategoryDescription(const Entities::IdType& id, 
                                                           const std::string& newDescription,
                                                           std::ostream& output) override;
            StatusCode changeDiscussionCategoryParent(const Entities::IdType& id, 
                                                      const Entities::IdType& newParentId,
                                                      std::ostream& output) override;
            StatusCode changeDiscussionCategoryDisplayOrder(const Entities::IdType& id, 
                                                            int_fast16_t newDisplayOrder,
                                                            std::ostream& output) override;
            StatusCode deleteDiscussionCategory(const Entities::IdType& id, std::ostream& output) override;

            StatusCode getDiscussionCategoryById(const Entities::IdType& id, std::ostream& output) const override;
            StatusCode getDiscussionCategories(std::ostream& output, RetrieveDiscussionCategoriesBy by) const override;
            StatusCode getDiscussionCategoriesFromRoot(std::ostream& output) const override;

            StatusCode addDiscussionTagToCategory(const Entities::IdType& tagId, 
                                                  const Entities::IdType& categoryId, 
                                                  std::ostream& output) override;
            StatusCode removeDiscussionTagFromCategory(const Entities::IdType& tagId, 
                                                       const Entities::IdType& categoryId, 
                                                       std::ostream& output) override;
        private:
            boost::u32regex validDiscussionCategoryNameRegex;
        };
    }
}
