#pragma once

#include <boost/iterator/transform_iterator.hpp>

#include <cassert>
#include <memory>

namespace Forum
{
    namespace Helpers
    {
        template <typename T, typename TCollection>
        class ConstCollectionAdapter final
        {
        public:
            ConstCollectionAdapter(const TCollection& collection) : collection_(collection) {}

            auto size() const { return collection_.size(); }

            auto begin() const { return cbegin(); };
            auto end() const { return cend(); };
            auto rbegin() const { return crbegin(); };
            auto rend() const { return crend(); };

            auto cbegin() const
            {
                return boost::make_transform_iterator(collection_.cbegin(), getPointer);
            };

            auto cend() const
            {
                return boost::make_transform_iterator(collection_.cend(), getPointer);
            };

            auto crbegin() const
            {
                return boost::make_transform_iterator(collection_.crbegin(), getPointer);
            };

            auto crend() const
            {
                return boost::make_transform_iterator(collection_.crend(), getPointer);
            };

            template <typename TSearchType>
            auto find(const TSearchType& value) const
            {
                return boost::make_transform_iterator(collection_.find(value), getPointer);
            }

        private:

            static const T* getPointer(const std::shared_ptr<T>& ptr)
            {
                if ( ! ptr)
                {
                    assert("Collections should not contain empty shared pointers");
                    return nullptr;
                }
                return ptr.get();
            }

            const TCollection& collection_;
        };

        template<typename TCollection>
        auto toConst(const TCollection& collection)
        {
            return ConstCollectionAdapter<typename std::remove_pointer<typename TCollection::value_type::element_type>::type,
                    TCollection>(collection);
        };
    }
}
