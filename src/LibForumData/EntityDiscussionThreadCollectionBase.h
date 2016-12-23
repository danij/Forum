#pragma once

#include <memory>
#include <string>
#include <type_traits>

#include <boost/noncopyable.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThread.h"
#include "StringHelpers.h"
#include "TypeHelpers.h"

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
            DECLARE_INTERFACE_MANDATORY_NO_COPY(DiscussionThreadCollectionBase);

            struct DiscussionThreadCollectionById {};
            struct DiscussionThreadCollectionByName {};
            struct DiscussionThreadCollectionByCreated {};
            struct DiscussionThreadCollectionByLastUpdated {};
            struct DiscussionThreadCollectionByMessageCount {};

            struct DiscussionThreadCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<DiscussionThreadCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionThread::id>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByName>,
                            const boost::multi_index::const_mem_fun<DiscussionThread, const std::string&,
                                    &DiscussionThread::name>, Forum::Helpers::StringAccentAndCaseInsensitiveLess>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByCreated>,
                            const boost::multi_index::const_mem_fun<Creatable, const Forum::Entities::Timestamp,
                                    &DiscussionThread::created>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByLastUpdated>,
                            const boost::multi_index::const_mem_fun<DiscussionThread, const Forum::Entities::Timestamp,
                                    &DiscussionThread::lastUpdated>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByMessageCount>,
                            const boost::multi_index::const_mem_fun<DiscussionMessageCollectionBase,
                                    std::result_of<decltype(&DiscussionThread::messageCount)(DiscussionThread*)>::type,
                                    &DiscussionThread::messageCount>>
            > {};

            typedef boost::multi_index_container<DiscussionThreadRef, DiscussionThreadCollectionIndices>
                    DiscussionThreadCollection;

            inline auto& threads() { return threads_; }
            inline auto  threadsById() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionById>()); }
            inline auto  threadsByName() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByName>()); }
            inline auto  threadsByCreated() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByCreated>()); }
            inline auto  threadsByLastUpdated() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByLastUpdated>()); }
            inline auto  threadsByMessageCount() const
                { return Helpers::toConst(threads_.get<DiscussionThreadCollectionByMessageCount>()); }

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
            void modifyDiscussionThread(const IdType& id, const std::function<void(DiscussionThread&)>& modifyFunction);
            /**
             * Safely deletes a discussion thread instance, removing it from all indexes it is registered in
             */
            virtual void deleteDiscussionThread(DiscussionThreadCollection::iterator iterator);
            /**
             * Safely deletes a discussion thread instance, removing it from all indexes it is registered in
             */
            void deleteDiscussionThread(const IdType& id);

        protected:
            DiscussionThreadCollection threads_;
        };
    }
}
