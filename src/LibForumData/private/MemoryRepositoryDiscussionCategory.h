/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionCategory final : public MemoryRepositoryBase,
                                                         public IDiscussionCategoryRepository,
                                                         public IDiscussionCategoryDirectWriteRepository
        {
        public:
            explicit MemoryRepositoryDiscussionCategory(MemoryStoreRef store,
                                                        Authorization::DiscussionCategoryAuthorizationRef authorization);

            StatusCode addNewDiscussionCategory(StringView name, Entities::IdTypeRef parentId,
                                                OutStream& output) override;
            StatusWithResource<Entities::DiscussionCategoryPtr>
                addNewDiscussionCategory(Entities::EntityCollection& collection, Entities::IdTypeRef id,
                                         StringView name, Entities::IdTypeRef parentId) override;
            StatusCode changeDiscussionCategoryName(Entities::IdTypeRef id, StringView newName,
                                                    OutStream& output) override;
            StatusCode changeDiscussionCategoryName(Entities::EntityCollection& collection,
                                                    Entities::IdTypeRef id, StringView newName) override;
            StatusCode changeDiscussionCategoryDescription(Entities::IdTypeRef id,
                                                           StringView newDescription,
                                                           OutStream& output) override;
            StatusCode changeDiscussionCategoryDescription(Entities::EntityCollection& collection,
                                                           Entities::IdTypeRef id, StringView newDescription) override;
            StatusCode changeDiscussionCategoryParent(Entities::IdTypeRef id,
                                                      Entities::IdTypeRef newParentId,
                                                      OutStream& output) override;
            StatusCode changeDiscussionCategoryParent(Entities::EntityCollection& collection,
                                                      Entities::IdTypeRef id, Entities::IdTypeRef newParentId) override;
            StatusCode changeDiscussionCategoryDisplayOrder(Entities::IdTypeRef id,
                                                            int_fast16_t newDisplayOrder,
                                                            OutStream& output) override;
            StatusCode changeDiscussionCategoryDisplayOrder(Entities::EntityCollection& collection,
                                                            Entities::IdTypeRef id, int_fast16_t newDisplayOrder) override;
            StatusCode deleteDiscussionCategory(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode deleteDiscussionCategory(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;

            StatusCode getDiscussionCategoryById(Entities::IdTypeRef id, OutStream& output) const override;
            StatusCode getDiscussionCategories(OutStream& output, RetrieveDiscussionCategoriesBy by) const override;
            StatusCode getDiscussionCategoriesFromRoot(OutStream& output) const override;

            StatusCode addDiscussionTagToCategory(Entities::IdTypeRef tagId, Entities::IdTypeRef categoryId,
                                                  OutStream& output) override;
            StatusCode addDiscussionTagToCategory(Entities::EntityCollection& collection,
                                                  Entities::IdTypeRef tagId, Entities::IdTypeRef categoryId) override;
            StatusCode removeDiscussionTagFromCategory(Entities::IdTypeRef tagId, Entities::IdTypeRef categoryId,
                                                       OutStream& output) override;
            StatusCode removeDiscussionTagFromCategory(Entities::EntityCollection& collection,
                                                       Entities::IdTypeRef tagId, Entities::IdTypeRef categoryId) override;

        private:
            StatusCode changeDiscussionCategoryName(Entities::EntityCollection& collection,
                                                    Entities::IdTypeRef id,
                                                    Entities::DiscussionCategory::NameType&& newName);

            Authorization::DiscussionCategoryAuthorizationRef authorization_;
        };
    }
}
