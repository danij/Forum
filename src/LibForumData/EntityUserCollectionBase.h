#pragma once

#include <string>
#include <memory>

#include "EntityDiscussionThreadCollectionBase.h"
#include "EntityDiscussionMessageCollectionBase.h"
#include "Entities.h"

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

            struct UserCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<UserCollectionById>,
            const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &User::id>>,
            boost::multi_index::ranked_unique<boost::multi_index::tag<UserCollectionByName>,
                    const boost::multi_index::const_mem_fun<User, const std::string&, &User::name>,
                    Helpers::StringAccentAndCaseInsensitiveLess>,
            boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByCreated>,
                    const boost::multi_index::const_mem_fun<Creatable, const Timestamp,
                            &User::created>>,
            boost::multi_index::ranked_non_unique<boost::multi_index::tag<UserCollectionByLastSeen>,
                    const boost::multi_index::const_mem_fun<User, const Timestamp,
                            &User::lastSeen>>
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
            void modifyUser(UserCollection::iterator iterator, const std::function<void(User&)>& modifyFunction);
            /**
             * Enables a safe modification of a user instance, refreshing all indexes the user is registered in
             */
            void modifyUser(const IdType& id, const std::function<void(User&)>& modifyFunction);
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            virtual void deleteUser(UserCollection::iterator iterator);
            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            void deleteUser(const IdType& id);

        protected:
            UserCollection users_;
        };
    }
}
