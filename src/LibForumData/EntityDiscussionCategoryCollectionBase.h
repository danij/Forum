#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionCategory.h"
#include "StringHelpers.h"
#include "TypeHelpers.h"

#include <boost/noncopyable.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <type_traits>

namespace Forum
{
    namespace Entities
    {
        /**
         * Base class for storing a collection of discussion categories
         * Using multiple inheritance instead of composition in order to allow easier customization of modify/delete behavior
         */
        struct DiscussionCategoryCollectionBase : private boost::noncopyable
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(DiscussionCategoryCollectionBase);

            struct DiscussionCategoryCollectionById {};
            struct DiscussionCategoryCollectionByName {};
            struct DiscussionCategoryCollectionByMessageCount {};
            struct DiscussionCategoryCollectionByDisplayOrderRootPriority {};

            struct DiscussionCategoryCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<DiscussionCategoryCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionCategory::id>>,
                    boost::multi_index::ranked_unique<boost::multi_index::tag<DiscussionCategoryCollectionByName>,
                            const boost::multi_index::const_mem_fun<DiscussionCategory, const std::string&,
                                    &DiscussionCategory::name>, Helpers::StringAccentAndCaseInsensitiveLess>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionCategoryCollectionByMessageCount>,
                            const boost::multi_index::const_mem_fun<DiscussionCategory, int_fast32_t,
                                    &DiscussionCategory::messageCount>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionCategoryCollectionByDisplayOrderRootPriority>,
                            const boost::multi_index::const_mem_fun<DiscussionCategory, int_fast16_t,
                                    &DiscussionCategory::displayOrderWithRootPriority>>
            > {};

            typedef boost::multi_index_container<DiscussionCategoryRef, DiscussionCategoryCollectionIndices>
                    DiscussionCategoryCollection;

            auto& categories() { return categories_; }
            auto  categoriesById() const
                { return Helpers::toConst(categories_.get<DiscussionCategoryCollectionById>()); }
            auto  categoriesByName() const
                { return Helpers::toConst(categories_.get<DiscussionCategoryCollectionByName>()); }
            auto  categoriesByMessageCount() const
                { return Helpers::toConst(categories_.get<DiscussionCategoryCollectionByMessageCount>()); }
            auto  categoriesByDisplayOrderRootPriority() const
                { return Helpers::toConst(categories_.get<DiscussionCategoryCollectionByDisplayOrderRootPriority>()); }

            /**
             * Enables a safe modification of a discussion category instance,
             * refreshing all indexes the category is registered in
             */
            virtual void modifyDiscussionCategory(DiscussionCategoryCollection::iterator iterator,
                                             const std::function<void(DiscussionCategory&)>& modifyFunction);
            /**
             * Enables a safe modification of a discussion category instance,
             * refreshing all indexes the category is registered in
             */
            void modifyDiscussionCategoryById(const IdType& id, const std::function<void(DiscussionCategory&)>& modifyFunction);
            /**
             * Safely deletes a discussion category instance, removing it from all indexes it is registered in
             */
            virtual DiscussionCategoryRef deleteDiscussionCategory(DiscussionCategoryCollection::iterator iterator);
            /**
             * Safely deletes a discussion category instance, removing it from all indexes it is registered in
             */
            DiscussionCategoryRef deleteDiscussionCategoryById(const IdType& id);

        protected:
            DiscussionCategoryCollection categories_;
        };
    }
}
