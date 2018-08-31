/*
Fast Forum Backend
Copyright (C) Daniel Jurcau

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

#include "FixedSizeBufferPool.h"

#include <boost/noncopyable.hpp>

namespace Http
{
    template<typename T>
    class FixedSizeObjectPool final : boost::noncopyable
    {
    public:
        explicit FixedSizeObjectPool(const size_t maxBufferCount) : bufferPool_(maxBufferCount)
        {
        }

        template<typename ...Args>
        T* getObject(Args&& ...constructorArgs)
        {
            auto buffer = bufferPool_.leaseBufferForManualRelease();
            if (nullptr == buffer)
            {
                return nullptr;
            }
            return new (buffer->data) T(std::forward<Args>(constructorArgs)...);
        }

        void returnObject(T* value)
        {
            if (nullptr == value)
            {
                return;
            }
            value->~T();
            bufferPool_.returnBuffer(value);
        }

    private:
        FixedSizeBufferPool<sizeof(T), alignof(T)> bufferPool_;
    };
}
