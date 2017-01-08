#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollectionBase.h"

#include <string>
#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct DiscussionCategory final : public Identifiable, public Creatable, public DiscussionThreadCollectionBase
        {
            const std::string& name() const { return name_; }
                  std::string& name()       { return name_; }

            const std::string& description() const { return description_; }
                  std::string& description()       { return description_; }

            const uint_fast16_t& displayOrder() const { return displayOrder_; }
                  uint_fast16_t& displayOrder()       { return displayOrder_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                Description,
                DisplayOrder,
                Parent
            };
            
        private:
            std::string name_;
            std::string description_;
            uint_fast16_t displayOrder_;
        };

        typedef std::shared_ptr<DiscussionCategory> DiscussionCategoryRef;
    }
}
