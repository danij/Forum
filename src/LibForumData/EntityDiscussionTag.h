#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollectionBase.h"

#include <string>
#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct DiscussionTag final : public Identifiable, public Creatable, public DiscussionThreadCollectionBase
        {
            const std::string& name() const { return name_; }
                  std::string& name()       { return name_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name
            };
            
        private:
            std::string name_;
        };

        typedef std::shared_ptr<DiscussionTag> DiscussionTagRef;
    }
}
