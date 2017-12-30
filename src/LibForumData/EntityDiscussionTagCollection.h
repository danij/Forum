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

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionTag.h"

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

            void stopBatchInsert();

            void prepareUpdateName(DiscussionTagPtr tag);
            void updateName(DiscussionTagPtr tag);

            void prepareUpdateThreadCount(DiscussionTagPtr tag);
            void updateThreadCount(DiscussionTagPtr tag);

            void prepareUpdateMessageCount(DiscussionTagPtr tag);
            void updateMessageCount(DiscussionTagPtr tag);

            auto count()          const { return byId_.size(); }

            auto byId()           const { return Helpers::toConst(byId_); }
            auto byName()         const { return Helpers::toConst(byName_); }
            auto byThreadCount()  const { return Helpers::toConst(byThreadCount_); }
            auto byMessageCount() const { return Helpers::toConst(byMessageCount_); }

            auto& byId()           { return byId_; }
            auto& byName()         { return byName_; }
            auto& byThreadCount()  { return byThreadCount_; }
            auto& byMessageCount() { return byMessageCount_; }

        private:
            HASHED_UNIQUE_COLLECTION(DiscussionTag, id) byId_;

            RANKED_UNIQUE_COLLECTION(DiscussionTag, name) byName_;
            RANKED_UNIQUE_COLLECTION_ITERATOR(byName_) byNameUpdateIt_;

            RANKED_COLLECTION(DiscussionTag, threadCount) byThreadCount_;
            RANKED_COLLECTION_ITERATOR(byThreadCount_) byThreadCountUpdateIt_;

            RANKED_COLLECTION(DiscussionTag, messageCount) byMessageCount_;
            RANKED_COLLECTION_ITERATOR(byMessageCount_) byMessageCountUpdateIt_;
        };
    }
}
