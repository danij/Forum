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

#include <type_traits>

namespace Forum
{
    namespace Entities
    {
        /**
         * Base class for storing a collection of discussion tags
         * Using multiple inheritance instead of composition in order to allow easier customization of modify/delete behavior
         */
        struct DiscussionTagCollectionBase : private boost::noncopyable
        {
            DECLARE_INTERFACE_MANDATORY_NO_COPY(DiscussionTagCollectionBase);

            struct DiscussionTagCollectionById {};
            struct DiscussionTagCollectionByName {};
            struct DiscussionTagCollectionByMessageCount {};

            struct DiscussionTagCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<DiscussionTagCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionTag::id>>,
                    boost::multi_index::ranked_unique<boost::multi_index::tag<DiscussionTagCollectionByName>,
                            const boost::multi_index::const_mem_fun<DiscussionTag, const std::string&,
                                    &DiscussionTag::name>, Helpers::StringAccentAndCaseInsensitiveLess>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionTagCollectionByMessageCount>,
                            const boost::multi_index::const_mem_fun<DiscussionTag, int_fast32_t,
                                    &DiscussionTag::messageCount>>
            > {};

            typedef boost::multi_index_container<DiscussionTagRef, DiscussionTagCollectionIndices>
                    DiscussionTagCollection;

            auto& tags() { return tags_; }
            auto  tagsById() const
                { return Helpers::toConst(tags_.get<DiscussionTagCollectionById>()); }
            auto  tagsByName() const
                { return Helpers::toConst(tags_.get<DiscussionTagCollectionByName>()); }
            auto  tagsByMessageCount() const
                { return Helpers::toConst(tags_.get<DiscussionTagCollectionByMessageCount>()); }

            /**
             * Enables a safe modification of a discussion tag instance,
             * refreshing all indexes the tag is registered in
             */
            virtual void modifyDiscussionTag(DiscussionTagCollection::iterator iterator,
                                             const std::function<void(DiscussionTag&)>& modifyFunction);
            /**
             * Enables a safe modification of a discussion tag instance,
             * refreshing all indexes the tag is registered in
             */
            void modifyDiscussionTagById(const IdType& id, const std::function<void(DiscussionTag&)>& modifyFunction);
            /**
             * Safely deletes a discussion tag instance, removing it from all indexes it is registered in
             */
            virtual DiscussionTagRef deleteDiscussionTag(DiscussionTagCollection::iterator iterator);
            /**
             * Safely deletes a discussion tag instance, removing it from all indexes it is registered in
             */
            DiscussionTagRef deleteDiscussionTagById(const IdType& id);

        protected:
            DiscussionTagCollection tags_;
        };
    }
}
