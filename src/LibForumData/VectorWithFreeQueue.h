#pragma once

#include <memory>
#include <vector>
#include <queue>

namespace Forum
{
    namespace Entities
    {
        template<typename T>
        class VectorWithFreeQueue final
        {
        public:
            typedef typename std::vector<T>::size_type IndexType;

            template<class... Args>
            IndexType add(Args&&... args)
            {
                auto ptr = std::make_unique<T>(std::forward<Args>(args)...);

                if (freeIndexes_.size())
                {
                    auto index = freeIndexes_.front();
                    freeIndexes_.pop();
                    vector_[index] = std::move(ptr);
                    return index;
                }

                vector_.push_back(std::move(ptr));
                return vector_.size() - 1;
            }

            void remove(IndexType index)
            {
                vector_[index].reset(nullptr);
                freeIndexes_.push(index);
            }

            auto data()
            {
                return vector_.data();
            }

        private:

#if defined(DEBUG) || defined(_DEBUG)
            std::vector<std::unique_ptr<T>> vector_;
#else
            static constexpr size_t InitialNumberOfItems = 131072;
            std::vector<std::unique_ptr<T>> vector_{ InitialNumberOfItems };
#endif
            std::queue<IndexType> freeIndexes_;
        };
    }
}
