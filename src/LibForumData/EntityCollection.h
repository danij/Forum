#pragma once

#include <string>
#include <memory>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/uuid/uuid.hpp>

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
            void modifyUser(const IdType& id, std::function<void(User&)> modifyFunction);

            /**
             * Safely deletes a user instance, removing it from all indexes it is registered in
             */
            void deleteUser(const IdType& id);

        private:
            UserCollection users_;
        };

        extern const User AnonymousUser;
    }
}
