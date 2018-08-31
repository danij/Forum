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

#include <memory>
#include <vector>
#include <queue>

namespace Forum::Entities
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

            if ( ! freeIndexes_.empty())
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

        static constexpr size_t InitialNumberOfItems = 131072;
        std::vector<std::unique_ptr<T>> vector_{ InitialNumberOfItems };

        std::queue<IndexType> freeIndexes_;
    };
}
