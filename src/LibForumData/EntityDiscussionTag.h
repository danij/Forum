#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollectionBase.h"

#include <string>
#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct DiscussionTag final : public Identifiable, public CreatedMixin, public DiscussionThreadCollectionBase
        {
            const std::string& name()         const { return name_; }
                  std::string& name()               { return name_; }

            const std::string& uiBlob()       const { return uiBlob_; }
                  std::string& uiBlob()             { return uiBlob_; }

            int_fast32_t       messageCount() const { return messageCount_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                UIBlob
            };
            
        private:
            std::string name_;
            std::string uiBlob_;
            int_fast32_t messageCount_ = 0;
        };

        typedef std::shared_ptr<DiscussionTag> DiscussionTagRef;
    }
}
