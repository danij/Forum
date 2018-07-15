/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "EntityPointer.h"
#include "UuidString.h"
#include "IpAddress.h"
#include "SortedVector.h"

#include <cstdint>
#include <limits>
#include <utility>
#include <type_traits>

#include <boost/preprocessor/cat.hpp>

namespace Forum::Entities
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

    constexpr Timestamp TimestampMin = std::numeric_limits<Timestamp>::min();
    constexpr Timestamp TimestampMax = std::numeric_limits<Timestamp>::max();

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

#define INDEX_CONST_MEM_FUN(Type, Getter) \
    const boost::multi_index::const_mem_fun<Type, typename std::result_of<decltype(&Type::Getter)(Type*)>::type, &Type::Getter>

#define HASHED_COLLECTION(Type, Getter) \
    boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by< \
        boost::multi_index::hashed_non_unique<INDEX_CONST_MEM_FUN(Type, Getter)>>>
#define HASHED_COLLECTION_ITERATOR(Member) decltype(Member)::nth_index<0>::type::iterator

#define HASHED_UNIQUE_COLLECTION(Type, Getter) \
    boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by< \
        boost::multi_index::hashed_unique<INDEX_CONST_MEM_FUN(Type, Getter)>>>
#define HASHED_UNIQUE_COLLECTION_ITERATOR(Member) decltype(Member)::nth_index<0>::type::iterator

#define ORDERED_COLLECTION(Type, Getter) \
    boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by< \
        boost::multi_index::ordered_non_unique<INDEX_CONST_MEM_FUN(Type, Getter)>>>
#define ORDERED_COLLECTION_ITERATOR(Member) decltype(Member)::nth_index<0>::type::iterator

#define ORDERED_UNIQUE_COLLECTION(Type, Getter) \
    boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by< \
        boost::multi_index::ordered_unique<INDEX_CONST_MEM_FUN(Type, Getter)>>>
#define ORDERED_UNIQUE_COLLECTION_ITERATOR(Member) decltype(Member)::nth_index<0>::type::iterator

#define RANKED_COLLECTION(Type, Getter) \
    boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by< \
        boost::multi_index::ranked_non_unique<INDEX_CONST_MEM_FUN(Type, Getter)>>>
#define RANKED_COLLECTION_ITERATOR(Member) decltype(Member)::nth_index<0>::type::iterator

#define RANKED_UNIQUE_COLLECTION(Type, Getter) \
    boost::multi_index_container<EntityPointer<Type>, boost::multi_index::indexed_by< \
        boost::multi_index::ranked_unique<INDEX_CONST_MEM_FUN(Type, Getter)>>>
#define RANKED_UNIQUE_COLLECTION_ITERATOR(Member) decltype(Member)::nth_index<0>::type::iterator

    template<typename TContainer, typename It, typename Value>
    void replaceItemInContainer(TContainer& container, It&& iterator, Value&& value)
    {
        container.replace(std::forward<It>(iterator), std::forward<Value>(value));
    }

#define GET_COMPARER(Getter) BOOST_PP_CAT(LessPtr_, Getter)
#define GET_COMPARER_FOR(T, Getter) BOOST_PP_CAT(LessPtr_, Getter)<T>

#define GET_EXTRACTOR(Getter) BOOST_PP_CAT(PtrExtractor_, Getter)
#define GET_EXTRACTOR_FOR(T, Getter) BOOST_PP_CAT(PtrExtractor_, Getter)<T>

#define SORTED_VECTOR_UNIQUE_COLLECTION(Type, Getter) \
    SortedVectorUnique<EntityPointer<Type>, \
                       std::remove_reference<std::remove_const< std::result_of<decltype(&Type::Getter)(Type)>::type >::type>::type, \
                       GET_EXTRACTOR_FOR(Type, Getter), GET_COMPARER_FOR(Type, Getter)>
#define SORTED_VECTOR_UNIQUE_COLLECTION_ITERATOR(Member) decltype(Member)::iterator

#define SORTED_VECTOR_COLLECTION(Type, Getter) \
    SortedVectorMultiValue<EntityPointer<Type>, \
                           std::remove_reference<std::remove_const< std::result_of<decltype(&Type::Getter)(Type)>::type >::type>::type, \
                           GET_EXTRACTOR_FOR(Type, Getter), GET_COMPARER_FOR(Type, Getter)>
#define SORTED_VECTOR_COLLECTION_ITERATOR(Member) decltype(Member)::iterator

#define DEFINE_PTR_COMPARER(Getter) \
    template<typename T> \
    struct GET_COMPARER(Getter) \
    { \
        using result_type = typename std::remove_reference<typename std::remove_const<typename std::result_of<decltype(&T::Getter)(T)>::type>::type>::type; \
        constexpr bool operator()(EntityPointer<T> first, EntityPointer<T> second) const \
        { \
            return first->Getter() < second->Getter(); \
        } \
        constexpr bool operator()(const result_type& first, EntityPointer<T> second) const \
        { \
            return first < second->Getter(); \
        } \
        constexpr bool operator()(EntityPointer<T> first, const result_type& second) const \
        { \
            return first->Getter() < second; \
        } \
    };

#define DEFINE_PTR_EXTRACTOR(Getter) \
    template<typename T> \
    struct GET_EXTRACTOR(Getter) \
    { \
        constexpr auto operator()(EntityPointer<T> value) const \
        { \
            return value->Getter(); \
        } \
    };

    DEFINE_PTR_EXTRACTOR(id)
    DEFINE_PTR_COMPARER(id)

    DEFINE_PTR_EXTRACTOR(name)
    DEFINE_PTR_COMPARER(name)

    DEFINE_PTR_EXTRACTOR(created)
    DEFINE_PTR_COMPARER(created)

    DEFINE_PTR_EXTRACTOR(lastUpdated)
    DEFINE_PTR_COMPARER(lastUpdated)

    DEFINE_PTR_EXTRACTOR(latestMessageCreated)
    DEFINE_PTR_COMPARER(latestMessageCreated)

    DEFINE_PTR_EXTRACTOR(messageCount)
    DEFINE_PTR_COMPARER(messageCount)

    DEFINE_PTR_EXTRACTOR(pinDisplayOrder)
    DEFINE_PTR_COMPARER(pinDisplayOrder)

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
        assert(false); //not found
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
