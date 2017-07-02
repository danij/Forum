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

            void prepareUpdateName(DiscussionCategoryPtr category);
            void updateName(DiscussionCategoryPtr category);

            void prepareUpdateMessageCount(DiscussionCategoryPtr category);
            void updateMessageCount(DiscussionCategoryPtr category);
            void refreshByMessageCount();
            
            void prepareUpdateDisplayOrderRootPriority(DiscussionCategoryPtr category);
            void updateDisplayOrderRootPriority(DiscussionCategoryPtr category);
            void refreshByDisplayOrderRootPriority();

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
            decltype(byName_)::nth_index<0>::type::iterator byNameUpdateIt_;

            RANKED_COLLECTION(DiscussionCategory, messageCount) byMessageCount_;
            decltype(byMessageCount_)::nth_index<0>::type::iterator byMessageCountUpdateIt_;

            ORDERED_COLLECTION(DiscussionCategory, displayOrderWithRootPriority) byDisplayOrderRootPriority_;
            decltype(byDisplayOrderRootPriority_)::nth_index<0>::type::iterator byDisplayOrderRootPriorityUpdateIt_;
        };
    }
}
