#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionTag.h"

#include <functional>

#include <boost/noncopyable.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Forum
{
    namespace Entities
    {
        class DiscussionTagCollection final : private boost::noncopyable
        {
        public:
            bool add(DiscussionTagPtr tag);
            bool remove(DiscussionTagPtr tag);

            void updateName(DiscussionTagPtr tag);
            void updateMessageCount(DiscussionTagPtr tag);

            auto& onCountChange()       { return onCountChange_; }

            auto count()          const { return byId_.size(); }

            auto byId()           const { return Helpers::toConst(byId_); }
            auto byName()         const { return Helpers::toConst(byName_); }
            auto byMessageCount() const { return Helpers::toConst(byMessageCount_); }

            auto& byId()           { return byId_; }
            auto& byName()         { return byName_; }
            auto& byMessageCount() { return byMessageCount_; }

        private:
            HASHED_UNIQUE_COLLECTION(DiscussionTag, id) byId_;

            RANKED_UNIQUE_COLLECTION(DiscussionTag, name) byName_;

            RANKED_COLLECTION(DiscussionTag, messageCount) byMessageCount_;

            std::function<void()> onCountChange_;
        };
    }
}
