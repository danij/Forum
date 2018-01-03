/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include <boost/iterator/transform_iterator.hpp>
#include <boost/optional.hpp>

#include <cassert>
#include <memory>
#include <map>
#include <unordered_map>
#include <set>

namespace Forum
{
    namespace Helpers
    {
        template <typename T, typename TCollection>
        class ConstSharedPointerCollectionAdapter final
        {
        public:
            explicit ConstSharedPointerCollectionAdapter(const TCollection& collection) : collection_(collection) {}

            auto size()   const { return collection_.size(); }

            auto begin()  const { return cbegin(); }
            auto end()    const { return cend(); }
            auto rbegin() const { return crbegin(); }
            auto rend()   const { return crend(); }

            auto cbegin() const
            {
                return boost::make_transform_iterator(collection_.cbegin(), getPointer);
            }

            auto cend() const
            {
                return boost::make_transform_iterator(collection_.cend(), getPointer);
            }

            auto crbegin() const
            {
                return boost::make_transform_iterator(collection_.crbegin(), getPointer);
            }

            auto crend() const
            {
                return boost::make_transform_iterator(collection_.crend(), getPointer);
            }

            auto nth(typename TCollection::size_type n) const
            {
                return boost::make_transform_iterator(collection_.nth(std::min(n, collection_.size())), getPointer);
            }

            template <typename TSearchType>
            auto find(const TSearchType& value) const
            {
                return boost::make_transform_iterator(collection_.find(value), getPointer);
            }

            template <typename TSearchType>
            auto lower_bound(const TSearchType& value) const
            {
                return boost::make_transform_iterator(collection_.lower_bound(value), getPointer);
            }

            template <typename TSearchType>
            auto lower_bound_rank(const TSearchType& value) const
            {
                return collection_.lower_bound_rank(value);
            }

        private:

            static const T* getPointer(Entities::EntityPointer<T> ptr)
            {
                assert(ptr); //collections should not contain empty pointers
                return ptr.ptr();
            }

            const TCollection& collection_;
        };

        template<typename TCollection>
        auto toConst(const TCollection& collection)
        {
            return ConstSharedPointerCollectionAdapter<typename std::remove_pointer<typename
                TCollection::value_type::element_type>::type, TCollection>(collection);
        }


        template <typename TKey, typename TValue, typename TCollection>
        class ConstMapAdapter final
        {
        public:
            explicit ConstMapAdapter(const TCollection& collection) : collection_(collection) {}

            auto size()   const { return collection_.size(); }

            auto begin()  const { return cbegin(); }
            auto end()    const { return cend(); }
            auto rbegin() const { return crbegin(); }
            auto rend()   const { return crend(); }

            auto cbegin() const
            {
                return boost::make_transform_iterator(collection_.cbegin(), getConstPair);
            }

            auto cend() const
            {
                return boost::make_transform_iterator(collection_.cend(), getConstPair);
            }

            auto crbegin() const
            {
                return boost::make_transform_iterator(collection_.crbegin(), getConstPair);
            }

            auto crend() const
            {
                return boost::make_transform_iterator(collection_.crend(), getConstPair);
            }

            template <typename TSearchType>
            auto find(const TSearchType& value) const
            {
                return boost::make_transform_iterator(collection_.find(value), getConstPair);
            }

        private:

            static auto getConstPair(const std::pair<TKey, TValue>& pair)
            {
                return std::make_pair(pair.first.toConst(), pair.second);
            }

            const TCollection& collection_;
        };

        template<typename MapKey, typename MapT, typename MapCompare, typename MapAllocator>
        auto toConst(const std::map<MapKey, MapT, MapCompare, MapAllocator>& collection)
        {
            return ConstMapAdapter<MapKey, MapT, std::map<MapKey, MapT, MapCompare, MapAllocator>>(collection);
        }

        template<typename MapKey, typename MapT, typename MapHash, typename MapKeyEqual, typename MapAllocator>
        auto toConst(const std::unordered_map<MapKey, MapT, MapHash, MapKeyEqual, MapAllocator>& collection)
        {
            return ConstMapAdapter<MapKey, MapT, std::unordered_map<MapKey, MapT, MapHash, MapKeyEqual, MapAllocator>>(collection);
        }
    }
}
