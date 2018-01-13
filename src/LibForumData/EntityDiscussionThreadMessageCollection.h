/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThreadMessage.h"

#include <functional>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

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
            bool add(DiscussionThreadMessageCollection& collection);
            bool remove(DiscussionThreadMessagePtr message);
            void clear();

            void stopBatchInsert();

            auto& onPrepareCountChange() { return onPrepareCountChange_; }
            auto& onCountChange()        { return onCountChange_; }

            auto count()          const { return byId_.size(); }

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

            SORTED_VECTOR_COLLECTION(DiscussionThreadMessage, created) byCreated_;

            std::function<void()> onPrepareCountChange_;
            std::function<void()> onCountChange_;
        };

        class DiscussionThreadMessageCollectionLowMemory final : private boost::noncopyable
        {
        public:
            bool add(DiscussionThreadMessagePtr message);
            bool add(DiscussionThreadMessageCollectionLowMemory& collection);
            bool remove(DiscussionThreadMessagePtr message);
            void clear();

            void stopBatchInsert();

            auto& onPrepareCountChange() { return onPrepareCountChange_; }
            auto& onCountChange()        { return onCountChange_; }

            auto count()          const { return byId_.size(); }
            auto empty()          const { return byId_.empty(); }

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
            SORTED_VECTOR_UNIQUE_COLLECTION(DiscussionThreadMessage, id) byId_;

            SORTED_VECTOR_COLLECTION(DiscussionThreadMessage, created) byCreated_;

            std::function<void()> onPrepareCountChange_;
            std::function<void()> onCountChange_;
        };
    }
}
