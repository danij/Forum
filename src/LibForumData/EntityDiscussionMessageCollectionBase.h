#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionMessage.h"
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
        struct DiscussionMessageCollectionBase : private boost::noncopyable
        {
            DECLARE_INTERFACE_MANDATORY_NO_COPY(DiscussionMessageCollectionBase);

            struct DiscussionMessageCollectionById {};
            struct DiscussionMessageCollectionByCreated {};

            struct DiscussionMessageCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<DiscussionMessageCollectionById>,
            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionMessage::id>>,
            boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionMessageCollectionByCreated>,
                    const boost::multi_index::const_mem_fun<CreatedMixin, const Timestamp&,
                            &DiscussionMessage::created>>
            > {};

            typedef boost::multi_index_container<DiscussionMessageRef, DiscussionMessageCollectionIndices>
                    DiscussionMessageCollection;

            auto& messages() { return messages_; }
            auto  messageCount() const { return messages_.size(); }
            auto  messagesById() const
            { return Helpers::toConst(messages_.get<DiscussionMessageCollectionById>()); }
            auto  messagesByCreated() const
            { return Helpers::toConst(messages_.get<DiscussionMessageCollectionByCreated>()); }

            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            virtual void modifyDiscussionMessage(DiscussionMessageCollection::iterator iterator,
                                                 const std::function<void(DiscussionMessage&)>& modifyFunction);
            /**
             * Enables a safe modification of a discussion message instance,
             * refreshing all indexes the message is registered in
             */
            void modifyDiscussionMessageById(const IdType& id, const std::function<void(DiscussionMessage&)>& modifyFunction);
            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            virtual void deleteDiscussionMessage(DiscussionMessageCollection::iterator iterator);
            /**
             * Safely deletes a discussion message instance, removing it from all indexes it is registered in
             */
            void deleteDiscussionMessageById(const IdType& id);

        protected:
            DiscussionMessageCollection messages_;
        };
    }
}
