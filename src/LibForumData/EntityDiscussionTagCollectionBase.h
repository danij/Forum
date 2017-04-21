#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionTag.h"
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
         * Base class for storing a collection of discussion tags
         * Using multiple inheritance instead of composition in order to allow easier customization of modify/delete behavior
         */
        template<typename IndexTypeForId>
        struct DiscussionTagCollectionBase : private boost::noncopyable
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(DiscussionTagCollectionBase);

            struct DiscussionTagCollectionById {};
            struct DiscussionTagCollectionByName {};
            struct DiscussionTagCollectionByMessageCount {};

            template<typename IndexType, typename T1, typename T2>
            struct IdIndexType : boost::multi_index::hashed_unique<T1, T2>
            {};

            template<typename T1, typename T2>
            struct IdIndexType<OrderedIndexForId, T1, T2> : boost::multi_index::ranked_unique<T1, T2>
            {};

            struct DiscussionTagCollectionIndices : boost::multi_index::indexed_by<

                    IdIndexType<IndexTypeForId, boost::multi_index::tag<DiscussionTagCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionTag::id>>,

                    boost::multi_index::ranked_unique<boost::multi_index::tag<DiscussionTagCollectionByName>,
                            const boost::multi_index::const_mem_fun<DiscussionTag, StringView,
                                    &DiscussionTag::name>, Helpers::StringAccentAndCaseInsensitiveLess>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionTagCollectionByMessageCount>,
                            const boost::multi_index::const_mem_fun<DiscussionTag, int_fast32_t,
                                    &DiscussionTag::messageCount>>
            > {};

            typedef boost::multi_index_container<DiscussionTagRef, DiscussionTagCollectionIndices>
                    DiscussionTagCollection;
            typedef typename DiscussionTagCollection::iterator TagIdIteratorType;


            auto& tags()
            {
                return tags_;
            }

            auto tagsById() const
            {
                return Helpers::toConst(tags_.template get<DiscussionTagCollectionById>());
            }

            auto tagsByName() const
            {
                return Helpers::toConst(tags_.template get<DiscussionTagCollectionByName>());
            }

            auto tagsByMessageCount() const
            {
                return Helpers::toConst(tags_.template get<DiscussionTagCollectionByMessageCount>());
            }

            /**
             * Enables a safe modification of a discussion tag instance,
             * refreshing all indexes the tag is registered in
             */
            virtual void modifyDiscussionTag(TagIdIteratorType iterator,
                                             std::function<void(DiscussionTag&)>&& modifyFunction)
            {
                if (iterator == tags_.end())
                {
                    return;
                }
                tags_.modify(iterator, [&modifyFunction](const DiscussionTagRef& tag)
                                       {
                                           if (tag && modifyFunction)
                                           {
                                               modifyFunction(*tag);
                                           }
                                       });
            }

            /**
             * Enables a safe modification of a discussion tag instance,
             * refreshing all indexes the tag is registered in
             */
            void modifyDiscussionTagById(const IdType& id, std::function<void(DiscussionTag&)>&& modifyFunction)
            {
                modifyDiscussionTag(tags_.template get<DiscussionTagCollectionById>().find(id),
                                    std::forward<std::function<void(DiscussionTag&)>>(modifyFunction));
            }

            /**
             * Safely deletes a discussion tag instance, removing it from all indexes it is registered in
             */
            virtual DiscussionTagRef deleteDiscussionTag(TagIdIteratorType iterator)
            {
                DiscussionTagRef result;
                if (iterator == tags_.end())
                {
                    return result;
                }
                result = *iterator;
                tags_.erase(iterator);
                return result;
            }

            /**
             * Safely deletes a discussion tag instance, removing it from all indexes it is registered in
             */
            DiscussionTagRef deleteDiscussionTagById(const IdType& id)
            {
                return deleteDiscussionTag(tags_.template get<DiscussionTagCollectionById>().find(id));
            }

        protected:
            DiscussionTagCollection tags_;
        };
    }
}
