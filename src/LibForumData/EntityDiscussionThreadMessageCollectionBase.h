#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThreadMessage.h"
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
         * Base class for storing a collection of discussion threads
         * Using multiple inheritance instead of composition in order to allow easier customization of modify/delete behavior
         */
        struct DiscussionThreadMessageCollectionBase : private boost::noncopyable
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(DiscussionThreadMessageCollectionBase);

            struct DiscussionThreadMessageCollectionById {};
            struct DiscussionThreadMessageCollectionByCreated {};

            struct DiscussionMessageCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<DiscussionThreadMessageCollectionById>,
                        const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionThreadMessage::id>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadMessageCollectionByCreated>,
                        const boost::multi_index::const_mem_fun<CreatedMixin, Timestamp, &DiscussionThreadMessage::created>>
            > {};

            typedef boost::multi_index_container<DiscussionThreadMessageRef, DiscussionMessageCollectionIndices>
                    DiscussionThreadMessageCollection;

            auto& messages() { return messages_; }
            auto  messageCount() const { return messages_.size(); }
            auto  messagesById() const
            { return Helpers::toConst(messages_.get<DiscussionThreadMessageCollectionById>()); }
            auto  messagesByCreated() const
            { return Helpers::toConst(messages_.get<DiscussionThreadMessageCollectionByCreated>()); }

            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            virtual void modifyDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator,
                                                       const std::function<void(DiscussionThreadMessage&)>& modifyFunction);
            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            void modifyDiscussionThreadMessageById(const IdType& id, const std::function<void(DiscussionThreadMessage&)>& modifyFunction);
            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            virtual DiscussionThreadMessageRef deleteDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator);
            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            DiscussionThreadMessageRef deleteDiscussionThreadMessageById(const IdType& id);

        protected:
            DiscussionThreadMessageCollection messages_;
        };
    }
}
