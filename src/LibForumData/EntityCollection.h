#pragma once

#include <string>
#include <memory>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include "ConstCollectionAdapter.h"
#include "Entities.h"
#include "StringHelpers.h"

namespace Forum
{
    namespace Entities
    {
        struct EntityCollection : private boost::noncopyable
        {
            struct UserCollectionById {};
            struct UserCollectionByName {};
            struct UserCollectionByCreated {};
            struct UserCollectionByLastSeen {};

            struct UserCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<UserCollectionById>,
                        const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &User::id>>,
                    boost::multi_index::ranked_unique<boost::multi_index::tag<UserCollectionByName>,
                        const boost::multi_index::const_mem_fun<User, const std::string&, &User::name>,
                            Forum::Helpers::StringAccentAndCaseInsensitiveLess>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByCreated>,
                        const boost::multi_index::const_mem_fun<Creatable, const Forum::Entities::Timestamp,
                                &User::created>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByLastSeen>,
                        const boost::multi_index::const_mem_fun<User, const Forum::Entities::Timestamp,
                                &User::lastSeen>, std::greater<const Forum::Entities::Timestamp>>
                    > {};

            typedef boost::multi_index_container<UserRef, UserCollectionIndices> UserCollection;

            inline auto& users()                 { return users_; }
            inline auto  usersById()       const { return Helpers::toConst(users_.get<UserCollectionById>()); }
            inline auto  usersByName()     const { return Helpers::toConst(users_.get<UserCollectionByName>()); }
            inline auto  usersByCreated()  const { return Helpers::toConst(users_.get<UserCollectionByCreated>()); }
            inline auto  usersByLastSeen() const { return Helpers::toConst(users_.get<UserCollectionByLastSeen>()); }

            /**
             * Enables a safe modification of a user instance, refreshing all indexes the user is registered in
             */
            void modifyUser(UserCollection::iterator iterator, std::function<void(User&)> modifyFunction);
            /**
             * Enables a safe modification of a user instance, refreshing all indexes the user is registered in
             */
            void modifyUser(const IdType& id, std::function<void(User&)> modifyFunction);
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            void deleteUser(UserCollection::iterator iterator);
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            void deleteUser(const IdType& id);


            struct DiscussionThreadCollectionById {};
            struct DiscussionThreadCollectionByName {};
            struct DiscussionThreadCollectionByCreated {};
            struct DiscussionThreadCollectionByLastUpdated {};

            struct DiscussionThreadCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<DiscussionThreadCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &DiscussionThread::id>>,
                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<DiscussionThreadCollectionByName>,
                            const boost::multi_index::const_mem_fun<DiscussionThread, const std::string&, &DiscussionThread::name>,
                            Forum::Helpers::StringAccentAndCaseInsensitiveLess>,
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
            void modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                        std::function<void(DiscussionThread&)> modifyFunction);
            /**
             * Enables a safe modification of a discussion thread instance,
             * refreshing all indexes the thread is registered in
             */
            void modifyDiscussionThread(const IdType& id, std::function<void(DiscussionThread&)> modifyFunction);
            /**
             * Safely deletes a discussion thread instance, removing it from all indexes it is registered in
             */
            void deleteDiscussionThread(DiscussionThreadCollection::iterator iterator);
            /**
             * Safely deletes a discussion thread instance, removing it from all indexes it is registered in
             */
            void deleteDiscussionThread(const IdType& id);

        private:
            UserCollection users_;
            DiscussionThreadCollection threads_;
        };

        extern const UserRef AnonymousUser;
    }
}
