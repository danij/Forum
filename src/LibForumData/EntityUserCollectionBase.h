#pragma once

#include "Entities.h"
#include "EntityDiscussionThreadMessageCollectionBase.h"

namespace Forum
{
    namespace Entities
    {
        /**
         * Base class for storing a collection of users
         * Using multiple inheritance instead of composition in order to allow easier customization of modify/delete behavior
         */
        template <typename IndexTypeForId>
        struct UserCollectionBase
        {
            DECLARE_ABSTRACT_MANDATORY_NO_COPY(UserCollectionBase)

            struct UserCollectionById {};
            struct UserCollectionByName {};
            struct UserCollectionByCreated {};
            struct UserCollectionByLastSeen {};
            struct UserCollectionByThreadCount {};
            struct UserCollectionByMessageCount {};

            template<typename IndexType, typename T1, typename T2>
            struct IdIndexType : boost::multi_index::hashed_unique<T1, T2>
            {};

            template<typename T1, typename T2>
            struct IdIndexType<OrderedIndexForId, T1, T2> : boost::multi_index::ranked_unique<T1, T2>
            {};

            struct UserCollectionIndices : boost::multi_index::indexed_by<

                    IdIndexType<IndexTypeForId, boost::multi_index::tag<UserCollectionById>,
                            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &User::id>>,

                    boost::multi_index::ranked_unique<boost::multi_index::tag<UserCollectionByName>,
                            const boost::multi_index::const_mem_fun<User, const std::string&, &User::name>,
                                Helpers::StringAccentAndCaseInsensitiveLess>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByCreated>,
                            const boost::multi_index::const_mem_fun<CreatedMixin, Timestamp, &User::created>>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByLastSeen>,
                            const boost::multi_index::const_mem_fun<User, Timestamp, &User::lastSeen>>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByThreadCount>,
                            const boost::multi_index::const_mem_fun<DiscussionThreadCollectionBase<OrderedIndexForId>,
                                size_t, &User::threadCount>>,

                    boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByMessageCount>,
                            const boost::multi_index::const_mem_fun<DiscussionThreadMessageCollectionBase<OrderedIndexForId>,
                                size_t, &User::messageCount>>
            > {};

            typedef boost::multi_index_container<UserRef, UserCollectionIndices> UserCollection;
            typedef typename UserCollection::iterator UserIdIteratorType;

            auto& users()                     { return users_; }
            auto  usersById()           const { return Helpers::toConst(users_.template get<UserCollectionById>()); }
            auto  usersByName()         const { return Helpers::toConst(users_.template get<UserCollectionByName>()); }
            auto  usersByCreated()      const { return Helpers::toConst(users_.template get<UserCollectionByCreated>()); }
            auto  usersByLastSeen()     const { return Helpers::toConst(users_.template get<UserCollectionByLastSeen>()); }
            auto  usersByThreadCount()  const { return Helpers::toConst(users_.template get<UserCollectionByThreadCount>()); }
            auto  usersByMessageCount() const { return Helpers::toConst(users_.template get<UserCollectionByMessageCount>()); }

            /**
             * Enables a safe modification of a user instance, refreshing all indexes the user is registered in
             */
            void modifyUser(UserIdIteratorType iterator, std::function<void(User&)>&& modifyFunction = {})
            {
                if (iterator == users_.end())
                {
                    return;
                }
                users_.modify(iterator, [&modifyFunction](const UserRef& user)
                                        {
                                            if (user && modifyFunction)
                                            {
                                                modifyFunction(*user);
                                            }
                                        });
            }
            
            /**
             * Enables a safe modification of a user instance, refreshing all indexes the user is registered in
             */
            void modifyUserById(const IdType& id, std::function<void(User&)>&& modifyFunction = {})
            {
                return modifyUser(users_.template get<UserCollectionById>().find(id),
                                  std::forward<std::function<void(User&)>>(modifyFunction));
            }

            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            virtual UserRef deleteUser(UserIdIteratorType iterator)
            {
                UserRef result;
                if (iterator == users_.end())
                {
                    return result;
                }
                result = *iterator;
                users_.erase(iterator);
                return result;
            }

            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            UserRef deleteUserById(const IdType& id)
            {
                return deleteUser(users_.template get<UserCollectionById>().find(id));
            }

        protected:
            UserCollection users_;
        };
    }
}
