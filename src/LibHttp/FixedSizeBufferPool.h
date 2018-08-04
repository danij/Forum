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

#include <cstddef>
#include <memory>
#include <mutex>

#include <boost/noncopyable.hpp>

namespace Http
{
    template<size_t BufferSize, size_t AlignSpecifier = 1>
    class FixedSizeBufferPool final : boost::noncopyable
    {
    public:
        struct Buffer
        {
            alignas(AlignSpecifier) char data[BufferSize];
        };

        struct BringBackBuffer final
        {
            void operator()(Buffer* toReturn)
            {
                if (manager) manager->returnBuffer(toReturn);
            }

            FixedSizeBufferPool* manager;
        };

        typedef std::unique_ptr<Buffer, BringBackBuffer> LeasedBufferType;

        explicit FixedSizeBufferPool(const size_t maxBufferCount) :
            maxBufferCount_(maxBufferCount), numberOfUsedBuffers_(0),
            buffers_(std::make_unique<Buffer[]>(maxBufferCount)),
            availableIndexes_(std::make_unique<size_t[]>(maxBufferCount))
        {
            for (size_t i = 0; i < maxBufferCount_; ++i)
            {
                availableIndexes_[i] = i;
            }
        }

        /**
         * Returns a buffer which must be manually returned to the pool
         */
        Buffer* leaseBufferForManualRelease()
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            if (numberOfUsedBuffers_ >= maxBufferCount_)
            {
                return {};
            }
            return &buffers_[availableIndexes_[numberOfUsedBuffers_++]];
        }

        /**
         * Returns a buffer that automatically returns to the pool on destruction
         */
        LeasedBufferType leaseBuffer()
        {
            return{ leaseBufferForManualRelease() , { this } };
        }

        void returnBuffer(void* dataPtr)
        {
            returnBuffer(reinterpret_cast<Buffer*>(reinterpret_cast<char*>(dataPtr) - offsetof(Buffer, data)));
        }

        void returnBuffer(Buffer* value)
        {
            if (nullptr == value)
            {
                return;
            }
            const size_t index = value - &buffers_[0];
            if (index >= maxBufferCount_)
            {
                return;
            }

            std::lock_guard<decltype(mutex_)> lock(mutex_);
            if (numberOfUsedBuffers_ < 1)
            {
                return;
            }
            availableIndexes_[--numberOfUsedBuffers_] = index;
        }

    private:
        const size_t maxBufferCount_;
        size_t numberOfUsedBuffers_;
        std::unique_ptr<Buffer[]> buffers_;
        std::unique_ptr<size_t[]> availableIndexes_;
        std::mutex mutex_;
    };
}
