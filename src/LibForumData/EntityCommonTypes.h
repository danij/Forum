#pragma once

#include "EntityPointer.h"
#include "UuidString.h"
#include "IpAddress.h"

#include <cstdint>

#include <boost/preprocessor/cat.hpp>

namespace Forum
{
    namespace Entities
    {
        /**
         * Using a string for representing the id to prevent constant conversions between string <-> uuid
         */
        typedef UuidString IdType;
        typedef const UuidString& IdTypeRef;

        /**
         * Representing a timestamp as the number of seconds since the UNIX EPOCH
         */
        typedef int_fast64_t Timestamp;

        struct VisitDetails final
        {
            Helpers::IpAddress ip;
        };

        struct LastUpdatedInfo final
        {
            Timestamp at{0};
            VisitDetails details;
            std::string reason;
            EntityPointer<User> by;
        };

#define HASHED_COLLECTION(Type, Getter) \
        boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by<boost::multi_index::hashed_non_unique< \
            const boost::multi_index::const_mem_fun<Type, typename std::result_of<decltype(&Type::Getter)(Type*)>::type, &Type::Getter>>>>

#define HASHED_UNIQUE_COLLECTION(Type, Getter) \
        boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by<boost::multi_index::hashed_unique< \
            const boost::multi_index::const_mem_fun<Type, typename std::result_of<decltype(&Type::Getter)(Type*)>::type, &Type::Getter>>>>

#define ORDERED_COLLECTION(Type, Getter) \
        boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by<boost::multi_index::ordered_non_unique< \
            const boost::multi_index::const_mem_fun<Type, typename std::result_of<decltype(&Type::Getter)(Type*)>::type, &Type::Getter>>>>

#define ORDERED_UNIQUE_COLLECTION(Type, Getter) \
        boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by<boost::multi_index::ordered_unique< \
            const boost::multi_index::const_mem_fun<Type, typename std::result_of<decltype(&Type::Getter)(Type*)>::type, &Type::Getter>>>>

#define RANKED_COLLECTION(Type, Getter) \
        boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by<boost::multi_index::ranked_non_unique< \
            const boost::multi_index::const_mem_fun<Type, typename std::result_of<decltype(&Type::Getter)(Type*)>::type, &Type::Getter>>>>

#define RANKED_UNIQUE_COLLECTION(Type, Getter) \
        boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by<boost::multi_index::ranked_unique< \
            const boost::multi_index::const_mem_fun<Type, typename std::result_of<decltype(&Type::Getter)(Type*)>::type, &Type::Getter>>>>

#define RETURN_TYPE(Type, Getter) \
        typename std::result_of<decltype(&Type::Getter)(Type*)>::type

#define FLAT_MULTISET_COLLECTION(Type, Getter) \
        boost::container::flat_multiset<EntityPointer<Type>, BOOST_PP_CAT(LessPtr_, Getter)<Type>>

        template<typename T>
        struct LessPtr_created
        {
            constexpr bool operator()(EntityPointer<T> lhs, EntityPointer<T> rhs) const
            {
                return lhs->created() < rhs->created();
            }
        };

        template<typename Collection, typename Entity, typename Value>
        void eraseFromNonUniqueCollection(Collection& collection, Entity toCompare, const Value& toSearch)
        {
            auto range = collection.equal_range(toSearch);
            for (auto it = range.first; it != range.second; ++it)
            {
                if (*it == toCompare)
                {
                    collection.erase(it);
                    return;
                }
            }
        }

        template<typename Collection, typename Entity>
        void eraseFromFlatMultisetCollection(Collection& collection, Entity toCompare)
        {
            auto range = collection.equal_range(toCompare);
            for (auto it = range.first; it != range.second; ++it)
            {
                if (*it == toCompare)
                {
                    collection.erase(it);
                    return;
                }
            }
        }

        template<typename Collection, typename Entity, typename Value>
        auto findInNonUniqueCollection(Collection& collection, Entity toCompare, const Value& toSearch)
        {
            auto range = collection.equal_range(toSearch);
            for (auto it = range.first; it != range.second; ++it)
            {
                if (*it == toCompare)
                {
                    return it;
                }
            }
            return collection.end();
        }
    }
}
