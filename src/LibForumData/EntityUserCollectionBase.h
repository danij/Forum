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
        struct UserCollectionBase
        {
            struct UserCollectionById {};
            struct UserCollectionByName {};
            struct UserCollectionByCreated {};
            struct UserCollectionByLastSeen {};
            struct UserCollectionByMessageCount {};

            struct UserCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<UserCollectionById>,
            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &User::id>>,
            boost::multi_index::ranked_unique<boost::multi_index::tag<UserCollectionByName>,
                    const boost::multi_index::const_mem_fun<User, const std::string&, &User::name>,
                    Helpers::StringAccentAndCaseInsensitiveLess>,
            boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByCreated>,
                    const boost::multi_index::const_mem_fun<CreatedMixin, Timestamp, &User::created>>,
            boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByLastSeen>,
                    const boost::multi_index::const_mem_fun<User, Timestamp, &User::lastSeen>>,
            boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByMessageCount>,
                    const boost::multi_index::const_mem_fun<DiscussionThreadMessageCollectionBase, 
                        std::result_of<decltype(&User::messageCount)(User*)>::type, &User::messageCount>>
            > {};

            typedef boost::multi_index_container<UserRef, UserCollectionIndices> UserCollection;

            auto& users()                     { return users_; }
            auto  usersById()           const { return Helpers::toConst(users_.get<UserCollectionById>()); }
            auto  usersByName()         const { return Helpers::toConst(users_.get<UserCollectionByName>()); }
            auto  usersByCreated()      const { return Helpers::toConst(users_.get<UserCollectionByCreated>()); }
            auto  usersByLastSeen()     const { return Helpers::toConst(users_.get<UserCollectionByLastSeen>()); }
            auto  usersByMessageCount() const { return Helpers::toConst(users_.get<UserCollectionByMessageCount>()); }

            /**
             * Enables a safe modification of a user instance, refreshing all indexes the user is registered in
             */
            void modifyUser(UserCollection::iterator iterator, const std::function<void(User&)>& modifyFunction);
            /**
             * Enables a safe modification of a user instance, refreshing all indexes the user is registered in
             */
            void modifyUserById(const IdType& id, const std::function<void(User&)>& modifyFunction);
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            virtual UserRef deleteUser(UserCollection::iterator iterator);
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            UserRef deleteUserById(const IdType& id);

        protected:
            UserCollection users_;
        };
    }
}
