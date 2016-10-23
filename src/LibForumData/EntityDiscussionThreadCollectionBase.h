#pragma once

#include <string>
#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/multi_index_container.hpp>

#include "ConstCollectionAdapter.h"
#include "EntityDiscussionThread.h"
#include "StringHelpers.h"
#include "TypeHelpers.h"

namespace Forum
{
    namespace Entities
    {
        struct DiscussionThreadCollectionBase : private boost::noncopyable
        {
            DECLARE_INTERFACE_MANDATORY_NO_COPY(DiscussionThreadCollectionBase);

            struct DiscussionThreadCollectionById {};
            struct DiscussionThreadCollectionByName {};
            struct DiscussionThreadCollectionByCreated {};
            struct DiscussionThreadCollectionByLastUpdated {};

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
                                    &DiscussionThread::lastUpdated>, std::greater<const Forum::Entities::Timestamp>>
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

            /**
             * Enables a safe modification of a discussion thread instance,
             * refreshing all indexes the thread is registered in
             */
            virtual void modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                        std::function<void(DiscussionThread&)> modifyFunction);
            /**
             * Enables a safe modification of a discussion thread instance,
             * refreshing all indexes the thread is registered in
             */
            void modifyDiscussionThread(const IdType& id, std::function<void(DiscussionThread&)> modifyFunction);
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