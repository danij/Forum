#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThreadMessage.h"
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
         * Base class for storing a collection of discussion threads
         * Using multiple inheritance instead of composition in order to allow easier customization of modify/delete behavior
         */
        template<typename IndexTypeForId>
        struct DiscussionThreadMessageCollectionBase : private boost::noncopyable
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(DiscussionThreadMessageCollectionBase);

            struct DiscussionThreadMessageCollectionById {};
            struct DiscussionThreadMessageCollectionByCreated {};

            template<typename IndexType, typename T1, typename T2>
            struct IdIndexType : boost::multi_index::hashed_unique<T1, T2> 
            {};

            template<typename T1, typename T2>
            struct IdIndexType<OrderedIndexForId, T1, T2> : boost::multi_index::ranked_unique<T1, T2>
            {};
            
            struct DiscussionMessageCollectionIndices : boost::multi_index::indexed_by<

                    IdIndexType<IndexTypeForId, boost::multi_index::tag<DiscussionThreadMessageCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionThreadMessage::id>>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadMessageCollectionByCreated>,
                            const boost::multi_index::const_mem_fun<CreatedMixin, Timestamp, &DiscussionThreadMessage::created>>
            > {};

            typedef boost::multi_index_container<DiscussionThreadMessageRef, DiscussionMessageCollectionIndices>
                    DiscussionThreadMessageCollection;
            typedef typename DiscussionThreadMessageCollection::iterator MessageIdIteratorType;

            auto& messages()
            {
                return messages_;
            }

            auto messageCount() const
            {
                return messages_.size();
            }

            auto messagesById() const
            {
                return Helpers::toConst(messages_.template get<DiscussionThreadMessageCollectionById>());
            }

            auto messagesByCreated() const
            {
                return Helpers::toConst(messages_.template get<DiscussionThreadMessageCollectionByCreated>());
            }

            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            virtual void modifyDiscussionThreadMessage(MessageIdIteratorType iterator,
                                                       std::function<void(DiscussionThreadMessage&)>&& modifyFunction)
            {
                if (iterator == messages_.end())
                {
                    return;
                }
                messages_.modify(iterator, [&modifyFunction](const DiscussionThreadMessageRef& message)
                                           {
                                               if (message && modifyFunction)
                                               {
                                                   modifyFunction(*message);
                                               }
                                           });
            }

            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            void modifyDiscussionThreadMessageById(const IdType& id, 
                                                   std::function<void(DiscussionThreadMessage&)>&& modifyFunction)
            {
                modifyDiscussionThreadMessage(messages_.template get<DiscussionThreadMessageCollectionById>().find(id),
                                              std::forward<std::function<void(DiscussionThreadMessage&)>>(modifyFunction));
            }

            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            virtual DiscussionThreadMessageRef deleteDiscussionThreadMessage(MessageIdIteratorType iterator)
            {
                DiscussionThreadMessageRef result;
                if (iterator == messages_.end())
                {
                    return result;
                }
                result = *iterator;
                messages_.erase(iterator);
                return result;
            }

            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            DiscussionThreadMessageRef deleteDiscussionThreadMessageById(const IdType& id)
            {
                return deleteDiscussionThreadMessage(messages_.template get<DiscussionThreadMessageCollectionById>().find(id));
            }

        protected:
            DiscussionThreadMessageCollection messages_;
        };
    }
}
