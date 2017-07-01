#pragma once

#include "EntityPointer.h"

#include <boost/iterator/transform_iterator.hpp>

#include <cassert>
#include <memory>
#include <map>
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
                return boost::make_transform_iterator(collection_.nth(n), getPointer);
            }

            template <typename TSearchType>
            auto find(const TSearchType& value) const
            {
                return boost::make_transform_iterator(collection_.find(value), getPointer);
            }

        private:

            static const T* getPointer(Entities::EntityPointer<T> ptr)
            {
                if ( ! ptr)
                {
                    assert("Collections should not contain empty shared pointers");
                    return nullptr;
                }
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

            //TODO take care of shared/weak_ptrs that point to non-const items
            static auto getConstPair(const std::pair<TKey, TValue>& pair)
            {
                return std::pair<typename std::add_const<TKey>::type, typename std::add_const<TValue>::type>
                    (pair.first, pair.second);
            }

            const TCollection& collection_;
        };

        template<typename MapKey, typename MapT, typename MapCompare, typename MapAllocator>
        auto toConst(const std::map<MapKey, MapT, MapCompare, MapAllocator>& collection)
        {
            return ConstMapAdapter<MapKey, MapT, std::map<MapKey, MapT, MapCompare, MapAllocator>>(collection);
        }

        template <typename T, typename TCollection>
        class ConstWeakPtrAdapter final
        {
        public:
            explicit ConstWeakPtrAdapter(const TCollection& collection) : collection_(collection) {}

            auto size()   const { return collection_.size(); }

            auto begin()  const { return cbegin(); }
            auto end()    const { return cend(); }
            auto rbegin() const { return crbegin(); }
            auto rend()   const { return crend(); }

            auto cbegin() const
            {
                return boost::make_transform_iterator(collection_.cbegin(), getSharedPtr);
            }

            auto cend() const
            {
                return boost::make_transform_iterator(collection_.cend(), getSharedPtr);
            }

            auto crbegin() const
            {
                return boost::make_transform_iterator(collection_.crbegin(), getSharedPtr);
            }

            auto crend() const
            {
                return boost::make_transform_iterator(collection_.crend(), getSharedPtr);
            }

            template <typename TSearchType>
            auto find(const TSearchType& value) const
            {
                return boost::make_transform_iterator(collection_.find(value), getSharedPtr);
            }

        private:

            //TODO take care of shared/weak_ptrs that point to non-const items
            static auto getSharedPtr(const std::weak_ptr<T>& weakPtr)
            {
                return weakPtr.lock();
            }

            const TCollection& collection_;
        };

        template<typename T, typename SetCompare, typename SetAllocator>
        auto toConst(const std::set<std::weak_ptr<T>, SetCompare, SetAllocator>& collection)
        {
            return ConstWeakPtrAdapter<T, std::set<std::weak_ptr<T>, SetCompare, SetAllocator>>(collection);
        }
    }
}
