#pragma once

#include <boost/iterator/transform_iterator.hpp>

namespace Forum
{
    namespace Helpers
    {
        template <typename T, typename TCollection>
        class ConstCollectionAdapter final
        {
        public:
            ConstCollectionAdapter(const TCollection& collection) : collection_(collection) {}

            inline auto size() const { return collection_.size(); }

            inline auto begin() const { return cbegin(); };
            inline auto end() const { return cend(); };

            inline auto cbegin() const
            {
                return boost::make_transform_iterator(collection_.cbegin(), getPointer);
            };

            inline auto cend() const
            {
                return boost::make_transform_iterator(collection_.cend(), getPointer);
            };

            template <typename TSearchType>
            inline auto find(const TSearchType& value) const
            {
                return boost::make_transform_iterator(collection_.find(value), getPointer);
            }

        private:

            inline static const T* getPointer(const std::shared_ptr<T>& ptr)
            {
                return ptr ? ptr.get() : nullptr;
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
