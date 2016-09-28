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

            struct UserCollectionIndices : boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<boost::multi_index::tag<UserCollectionById>,
                    const boost::multi_index::const_mem_fun<Identifiable, const IdType&, &User::id>>,
                                           boost::multi_index::ranked_unique<boost::multi_index::tag<UserCollectionByName>,
                    const boost::multi_index::const_mem_fun<User, const std::string&, &User::name>,
                                           Forum::Helpers::StringAccentAndCaseInsensitiveLess>
                    > {};

            typedef boost::multi_index_container<UserRef, UserCollectionIndices> UserCollection;

            inline auto& users()             { return users_; }
            inline auto  usersById()   const { return Helpers::toConst(users_.get<UserCollectionById>()); }
            inline auto  usersByName() const { return Helpers::toConst(users_.get<UserCollectionByName>()); }

        private:
            UserCollection users_;
        };
    }
}
