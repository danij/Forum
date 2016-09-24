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

            inline static const T& getReference(const std::shared_ptr<T>& ptr)
            {
                return *ptr;
            }

            inline auto cbegin() const
            {
                return boost::make_transform_iterator(collection_.cbegin(), getReference);
            };

            inline auto cend() const
            {
                return boost::make_transform_iterator(collection_.cend(), getReference);
            };

        private:
            const TCollection& collection_;
        };

        template<typename TCollection>
        auto toConst(const TCollection& collection)
        {
            return ConstCollectionAdapter<typename std::remove_pointer<decltype(collection.cbegin()->get())>::type,
                    TCollection>(collection);
        };
    }
}
