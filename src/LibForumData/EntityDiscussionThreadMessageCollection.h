#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThreadMessage.h"

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
        class DiscussionThreadMessageCollection final : private boost::noncopyable
        {
        public:
            bool add(DiscussionThreadMessagePtr message);
            bool remove(DiscussionThreadMessagePtr message);
            void clear();

            auto& onCountChange()       { return onCountChange_; }

            auto count()          const { return byId_.size(); }

            auto byId()           const { return Helpers::toConst(byId_); }
            auto byCreated()      const { return Helpers::toConst(byCreated_); }

            auto& byId()      { return byId_; }
            auto& byCreated() { return byCreated_; }

        private:
            HASHED_UNIQUE_COLLECTION(DiscussionThreadMessage, id) byId_;

            RANKED_COLLECTION(DiscussionThreadMessage, created) byCreated_;

            std::function<void()> onCountChange_;
        };
    }
}
