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
        struct MessageCommentCollectionBase : private boost::noncopyable
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(MessageCommentCollectionBase);

            struct MessageCommentCollectionById {};
            struct MessageCommentCollectionByCreated {};

            struct DiscussionMessageCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<MessageCommentCollectionById>,
                        const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &MessageComment::id>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<MessageCommentCollectionByCreated>,
                        const boost::multi_index::const_mem_fun<CreatedMixin, Timestamp, &MessageComment::created>>
            > {};

            typedef boost::multi_index_container<MessageCommentRef, DiscussionMessageCollectionIndices>
                    MessageCommentCollection;

            auto& messageComments() { return messageComments_; }
            auto  messageCommentCount() const { return messageComments_.size(); }
            auto  messageCommentsById() const
            { return Helpers::toConst(messageComments_.get<MessageCommentCollectionById>()); }
            auto  messageCommentsByCreated() const
            { return Helpers::toConst(messageComments_.get<MessageCommentCollectionByCreated>()); }

            /**
             * Enables a safe modification of a message comment instance,
             * refreshing all indexes the message is registered in
             */
            virtual void modifyMessageComment(MessageCommentCollection::iterator iterator,
                                              std::function<void(MessageComment&)>&& modifyFunction = {});
            /**
             * Enables a safe modification of a message comment instance,
             * refreshing all indexes the message is registered in
             */
            void modifyMessageCommentById(const IdType& id, std::function<void(MessageComment&)>&& modifyFunction = {});
            /**
             * Safely deletes a message comment instance, removing it from all indexes it is registered in
             */
            virtual MessageCommentRef deleteMessageComment(MessageCommentCollection::iterator iterator);
            /**
             * Safely deletes a message comment instance, removing it from all indexes it is registered in
             */
            MessageCommentRef deleteMessageCommentById(const IdType& id);

        protected:
            MessageCommentCollection messageComments_;
        };
    }
}
