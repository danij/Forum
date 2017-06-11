#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityMessageComment.h"
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
        template<typename IndexTypeForId>
        struct MessageCommentCollectionBase : private boost::noncopyable
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(MessageCommentCollectionBase);

            struct MessageCommentCollectionById {};
            struct MessageCommentCollectionByCreated {};

            template<typename IndexType, typename T1, typename T2>
            struct IdIndexType : boost::multi_index::hashed_unique<T1, T2>
            {};

            template<typename T1, typename T2>
            struct IdIndexType<OrderedIndexForId, T1, T2> : boost::multi_index::ranked_unique<T1, T2>
            {};

            struct MessageCommentCollectionIndices : boost::multi_index::indexed_by<

                    IdIndexType<IndexTypeForId, boost::multi_index::tag<MessageCommentCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &MessageComment::id>>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<MessageCommentCollectionByCreated>,
                            const boost::multi_index::const_mem_fun<CreatedMixin, Timestamp, &MessageComment::created>>
            > {};

            typedef boost::multi_index_container<MessageCommentRef, MessageCommentCollectionIndices>
                    MessageCommentCollection;
            typedef typename MessageCommentCollection::iterator MessageCommentIdIteratorType;

            auto& messageComments()
            {
                return messageComments_;
            }

            auto messageCommentCount() const
            {
                return messageComments_.size();
            }

            auto messageCommentsById() const
            {
                return Helpers::toConst(messageComments_.template get<MessageCommentCollectionById>());
            }

            auto messageCommentsByCreated() const
            {
                return Helpers::toConst(messageComments_.template get<MessageCommentCollectionByCreated>());
            }

            /**
             * Enables a safe modification of a message comment instance,
             * refreshing all indexes the message is registered in
             */
            virtual void modifyMessageComment(MessageCommentIdIteratorType iterator,
                                              std::function<void(MessageComment&)>&& modifyFunction)
            {
                if (iterator == messageComments_.end())
                {
                    return;
                }
                messageComments_.modify(iterator, [&modifyFunction](const MessageCommentRef& comment)
                                                  {
                                                      if (comment && modifyFunction)
                                                      {
                                                          modifyFunction(*comment);
                                                      }
                                                  });
            }

            /**
             * Enables a safe modification of a message comment instance,
             * refreshing all indexes the message is registered in
             */
            void modifyMessageCommentById(const IdType& id, std::function<void(MessageComment&)>&& modifyFunction)
            {
                modifyMessageComment(messageComments_.template get<MessageCommentCollectionById>().find(id),
                                     std::forward<std::function<void(MessageComment&)>>(modifyFunction));
            }

            /**
             * Safely deletes a message comment instance, removing it from all indexes it is registered in
             */
            virtual MessageCommentRef deleteMessageComment(MessageCommentIdIteratorType iterator)
            {
                MessageCommentRef result;
                if (iterator == messageComments_.end())
                {
                    return result;
                }
                result = *iterator;
                messageComments_.erase(iterator);
                return result;
            }

            /**
             * Safely deletes a message comment instance, removing it from all indexes it is registered in
             */
            MessageCommentRef deleteMessageCommentById(const IdType& id)
            {
                return deleteMessageComment(messageComments_.template get<MessageCommentCollectionById>().find(id));
            }

        protected:
            MessageCommentCollection messageComments_;
        };
    }
}
