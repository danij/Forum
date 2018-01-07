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
#include "EntityDiscussionCategory.h"

#include <boost/noncopyable.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Forum
{
    namespace Entities
    {
        class DiscussionCategoryCollection final : private boost::noncopyable
        {
        public:
            bool add(DiscussionCategoryPtr category);
            bool remove(DiscussionCategoryPtr category);

            void stopBatchInsert();

            void prepareUpdateName(DiscussionCategoryPtr category);
            void updateName(DiscussionCategoryPtr category);

            void prepareUpdateMessageCount(DiscussionCategoryPtr category);
            void updateMessageCount(DiscussionCategoryPtr category);

            void prepareUpdateDisplayOrderRootPriority(DiscussionCategoryPtr category);
            void updateDisplayOrderRootPriority(DiscussionCategoryPtr category);

            auto count()                      const { return byId_.size(); }

            auto byId()                       const { return Helpers::toConst(byId_); }
            auto byName()                     const { return Helpers::toConst(byName_); }
            auto byMessageCount()             const { return Helpers::toConst(byMessageCount_); }
            auto byDisplayOrderRootPriority() const { return Helpers::toConst(byDisplayOrderRootPriority_); }

            auto& byId()                       { return byId_; }
            auto& byName()                     { return byName_; }
            auto& byMessageCount()             { return byMessageCount_; }
            auto& byDisplayOrderRootPriority() { return byDisplayOrderRootPriority_; }

        private:
            HASHED_UNIQUE_COLLECTION(DiscussionCategory, id) byId_;

            RANKED_UNIQUE_COLLECTION(DiscussionCategory, name) byName_;
            RANKED_UNIQUE_COLLECTION_ITERATOR(byName_) byNameUpdateIt_;

            RANKED_COLLECTION(DiscussionCategory, messageCount) byMessageCount_;
            RANKED_COLLECTION_ITERATOR(byMessageCount_) byMessageCountUpdateIt_;

            ORDERED_COLLECTION(DiscussionCategory, displayOrderWithRootPriority) byDisplayOrderRootPriority_;
            ORDERED_COLLECTION_ITERATOR(byDisplayOrderRootPriority_) byDisplayOrderRootPriorityUpdateIt_;
        };
    }
}
