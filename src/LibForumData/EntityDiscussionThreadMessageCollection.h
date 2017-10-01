#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThreadMessage.h"

#include <functional>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/container/flat_set.hpp>

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

            void stopBatchInsert();

            auto& onPrepareCountChange() { return onPrepareCountChange_; }
            auto& onCountChange()        { return onCountChange_; }

            typedef RETURN_TYPE(HASHED_UNIQUE_COLLECTION(DiscussionThreadMessage, id), size) CountType;
            CountType count()     const { return byId_.size(); }

            auto byId()           const { return Helpers::toConst(byId_); }
            auto byCreated()      const { return Helpers::toConst(byCreated_); }

            auto& byId()      { return byId_; }
            auto& byCreated() { return byCreated_; }

            boost::optional<size_t> findRankByCreated(IdTypeRef messageId) const
            {
                auto idIt = byId_.find(messageId);
                if (idIt == byId_.end())
                {
                    return{};
                }
                auto range = byCreated_.equal_range(*idIt);
                for (auto it = range.first; it != range.second; ++it)
                {
                    if (*it == *idIt)
                    {
                        return byCreated_.index_of(it);
                    }
                }
                return{};
            }

        private:
            HASHED_UNIQUE_COLLECTION(DiscussionThreadMessage, id) byId_;

            FLAT_MULTISET_COLLECTION(DiscussionThreadMessage, created) byCreated_;

            std::function<void()> onPrepareCountChange_;
            std::function<void()> onCountChange_;
        };
    }
}
