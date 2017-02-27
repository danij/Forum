#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThread.h"
#include "StringHelpers.h"
#include "TypeHelpers.h"

#include <boost/noncopyable.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <memory>
#include <string>
#include <type_traits>

namespace Forum
{
    namespace Entities
    {
        /**
         * Base class for storing a collection of discussion threads
         * Using multiple inheritance instead of composition in order to allow easier customization of modify/delete behavior
         */
        struct DiscussionThreadCollectionBase : private boost::noncopyable
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(DiscussionThreadCollectionBase);

            struct DiscussionThreadCollectionById {};
            struct DiscussionThreadCollectionByName {};
            struct DiscussionThreadCollectionByCreated {};
            struct DiscussionThreadCollectionByLastUpdated {};
            struct DiscussionThreadCollectionByLatestMessageCreated {};
            struct DiscussionThreadCollectionByMessageCount {};

            struct DiscussionThreadCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<DiscussionThreadCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionThread::id>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByName>,
                            const boost::multi_index::const_mem_fun<DiscussionThread, const std::string&,
                                    &DiscussionThread::name>, Helpers::StringAccentAndCaseInsensitiveLess>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByCreated>,
                            const boost::multi_index::const_mem_fun<CreatedMixin, Timestamp,
                                    &DiscussionThread::created>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByLastUpdated>,
                            const boost::multi_index::const_mem_fun<LastUpdatedMixin<User>, Timestamp, 
                                    &DiscussionThread::lastUpdated>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByLatestMessageCreated>,
                            const boost::multi_index::const_mem_fun<DiscussionThread, Timestamp,
                                    &DiscussionThread::latestMessageCreated>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByMessageCount>,
                            const boost::multi_index::const_mem_fun<DiscussionThreadMessageCollectionBase,
                                    std::result_of<decltype(&DiscussionThread::messageCount)(DiscussionThread*)>::type,
                                    &DiscussionThread::messageCount>>
            > {};

            typedef boost::multi_index_container<DiscussionThreadRef, DiscussionThreadCollectionIndices>
                    DiscussionThreadCollection;

            auto& threads() { return threads_; }
            auto  threadsById() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionById>()); }
            auto  threadsByName() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByName>()); }
            auto  threadsByCreated() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByCreated>()); }
            auto  threadsByLastUpdated() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByLastUpdated>()); }
            auto  threadsByLatestMessageCreated() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByLatestMessageCreated>()); }
            auto  threadsByMessageCount() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByMessageCount>()); }

            bool containsThread(const DiscussionThreadRef& thread) const
            {
                if ( ! thread)
                {
                    return false;
                }
                return threads_.find(thread->id()) != threads_.end();
            }

            /**
             * Inserts a thread into a collection, returning false if the thread was already present
             */
            virtual bool insertDiscussionThread(const DiscussionThreadRef& thread);
            /**
             * Enables a safe modification of a discussion thread instance,
             * refreshing all indexes the thread is registered in
             */
            virtual void modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                                const std::function<void(DiscussionThread&)>& modifyFunction);
            /**
             * Enables a safe modification of a discussion thread instance,
             * refreshing all indexes the thread is registered in
             */
            void modifyDiscussionThreadById(const IdType& id, const std::function<void(DiscussionThread&)>& modifyFunction);
            /**
             * Safely deletes a discussion thread instance, removing it from all indexes it is registered in
             */
            virtual DiscussionThreadRef deleteDiscussionThread(DiscussionThreadCollection::iterator iterator);
            /**
             * Safely deletes a discussion thread instance, removing it from all indexes it is registered in
             */
            DiscussionThreadRef deleteDiscussionThreadById(const IdType& id);

        protected:
            DiscussionThreadCollection threads_;
        };
    }
}
