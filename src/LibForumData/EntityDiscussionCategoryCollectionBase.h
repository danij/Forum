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

namespace Forum
{
    namespace Entities
    {
        /**
         * Base class for storing a collection of discussion categories
         * Using multiple inheritance instead of composition in order to allow easier customization of modify/delete behavior
         */
        template <typename IndexTypeForId>
        struct DiscussionCategoryCollectionBase : private boost::noncopyable
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(DiscussionCategoryCollectionBase);

            struct DiscussionCategoryCollectionById {};
            struct DiscussionCategoryCollectionByName {};
            struct DiscussionCategoryCollectionByMessageCount {};
            struct DiscussionCategoryCollectionByDisplayOrderRootPriority {};

            template<typename IndexType, typename T1, typename T2>
            struct IdIndexType : boost::multi_index::hashed_unique<T1, T2>
            {};

            template<typename T1, typename T2>
            struct IdIndexType<OrderedIndexForId, T1, T2> : boost::multi_index::ranked_unique<T1, T2>
            {};

            struct DiscussionCategoryCollectionIndices : boost::multi_index::indexed_by<

                    IdIndexType<IndexTypeForId, boost::multi_index::tag<DiscussionCategoryCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionCategory::id>>,

                    boost::multi_index::ranked_unique<boost::multi_index::tag<DiscussionCategoryCollectionByName>,
                            const boost::multi_index::const_mem_fun<DiscussionCategory, const Helpers::StringWithSortKey&,
                                    &DiscussionCategory::name>>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionCategoryCollectionByMessageCount>,
                            const boost::multi_index::const_mem_fun<DiscussionCategory, int_fast32_t,
                                    &DiscussionCategory::messageCount>>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionCategoryCollectionByDisplayOrderRootPriority>,
                            const boost::multi_index::const_mem_fun<DiscussionCategory, int_fast16_t,
                                    &DiscussionCategory::displayOrderWithRootPriority>>
            > {};

            typedef boost::multi_index_container<DiscussionCategoryRef, DiscussionCategoryCollectionIndices>
                    DiscussionCategoryCollection;
            typedef typename DiscussionCategoryCollection::iterator CategoryIdIteratorType;


            auto& categories()
            {
                return categories_;
            }

            auto categoriesById() const
            {
                return Helpers::toConst(categories_.template get<DiscussionCategoryCollectionById>());
            }

            auto categoriesByName() const
            {
                return Helpers::toConst(categories_.template get<DiscussionCategoryCollectionByName>());
            }

            auto categoriesByMessageCount() const
            {
                return Helpers::toConst(categories_.template get<DiscussionCategoryCollectionByMessageCount>());
            }

            auto categoriesByDisplayOrderRootPriority() const
            {
                return Helpers::toConst(categories_.template get<DiscussionCategoryCollectionByDisplayOrderRootPriority>());
            }

            /**
             * Enables a safe modification of a discussion category instance,
             * refreshing all indexes the category is registered in
             */
            virtual void modifyDiscussionCategory(CategoryIdIteratorType iterator,
                                                  std::function<void(DiscussionCategory&)>&& modifyFunction)
            {
                if (iterator == categories_.end())
                {
                    return;
                }
                categories_.modify(iterator, [&modifyFunction](const DiscussionCategoryRef& category)
                                             {
                                                 if (category && modifyFunction)
                                                 {
                                                     modifyFunction(*category);
                                                 }
                                             });
            }

            /**
             * Enables a safe modification of a discussion category instance,
             * refreshing all indexes the category is registered in
             */
            void modifyDiscussionCategoryById(const IdType& id,
                                              std::function<void(DiscussionCategory&)>&& modifyFunction)
            {
                modifyDiscussionCategory(categories_.template get<DiscussionCategoryCollectionById>().find(id),
                                         std::forward<std::function<void(DiscussionCategory&)>>(modifyFunction));
            }

            /**
             * Safely deletes a discussion category instance, removing it from all indexes it is registered in
             */
            virtual DiscussionCategoryRef deleteDiscussionCategory(CategoryIdIteratorType iterator)
            {
                DiscussionCategoryRef result;
                if (iterator == categories_.end())
                {
                    return result;
                }
                result = *iterator;
                categories_.erase(iterator);
                return result;
            }

            /**
             * Safely deletes a discussion category instance, removing it from all indexes it is registered in
             */
            DiscussionCategoryRef deleteDiscussionCategoryById(const IdType& id)
            {
                return deleteDiscussionCategory(categories_.template get<DiscussionCategoryCollectionById>().find(id));
            }

        protected:
            DiscussionCategoryCollection categories_;
        };
    }
}
