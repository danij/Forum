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

#include <algorithm>
#include <array>
#include <cstddef>

#include <boost/noncopyable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace Http
{
    template<size_t BufferSize, size_t MaxNrOfBuffers>
    class ReadWriteBufferArray final : boost::noncopyable
    {
    public:
        explicit ReadWriteBufferArray(FixedSizeBufferPool<BufferSize>& bufferPool) : bufferPool_(bufferPool)
        {}

        /**
         * Appends data
         * @return true if there was enough room to copy the data, false otherwise
         */
        bool write(const char* input, size_t size)
        {
            if (0 == size) return true;

            while (size)
            {
                auto remainingSpaceInCurrentBuffer = BufferSize - usedBytesInLatestBuffer_;
                if (latestBuffer_ >= 0 && (remainingSpaceInCurrentBuffer >= size))
                {
                    //enough room in current buffer
                    std::copy(input, input + size, buffers_[latestBuffer_]->data + usedBytesInLatestBuffer_);
                    usedBytesInLatestBuffer_ += size;
                    return true;
                }
                if ((latestBuffer_ < 0) || (remainingSpaceInCurrentBuffer < 1))
                {
                    //need a new buffer
                    if ( ! requestNewBuffer())
                    {
                        //no more room available
                        notEnoughRoom_ = true;
                        return false;
                    }
                    remainingSpaceInCurrentBuffer = BufferSize;
                }
                auto toCopy = std::min(remainingSpaceInCurrentBuffer, size);
                std::copy(input, input + toCopy, buffers_[latestBuffer_]->data + usedBytesInLatestBuffer_);
                input += toCopy;
                size -= toCopy;
                usedBytesInLatestBuffer_ += toCopy;
            }

            return true;
        }

        /**
         * Returns the size of data stored in the buffers
         */
        size_t size() const
        {
            if (latestBuffer_ < 0) return 0;

            return latestBuffer_ * BufferSize + usedBytesInLatestBuffer_;
        }

        bool notEnoughRoom() const
        {
            return notEnoughRoom_;
        }

        /**
         * Enables reuse of the object instance
         */
        void reset()
        {
            if (latestBuffer_ >= 0)
            {
                for (int i = 0; i <= latestBuffer_; ++i)
                {
                    buffers_[i] = {};
                }
            }
            latestBuffer_ = -1;
            usedBytesInLatestBuffer_ = 0;
            notEnoughRoom_ = false;
        }

        typedef typename FixedSizeBufferPool<BufferSize>::LeasedBufferType BufferType;

        // implement the boost asio ConstBufferSequence concept

        struct ConstBufferWrapper
        {
            explicit ConstBufferWrapper(const ReadWriteBufferArray* bufferArray) : bufferArray_(bufferArray)
            {}

            virtual ~ConstBufferWrapper() = default;

            ConstBufferWrapper(const ConstBufferWrapper& other) = default;
            ConstBufferWrapper(ConstBufferWrapper&& other) = default;
            ConstBufferWrapper& operator=(const ConstBufferWrapper& other) = default;
            ConstBufferWrapper& operator=(ConstBufferWrapper&& other) = default;

            typedef boost::asio::const_buffer value_type;

            struct const_iterator : 
                boost::iterator_facade<const_iterator, value_type, boost::random_access_traversal_tag, value_type>
            {
                const_iterator() : bufferArray_(nullptr), currentIndex_(0)
                {}

                explicit const_iterator(const ReadWriteBufferArray* bufferArray, int startIndex)
                    : bufferArray_(bufferArray), currentIndex_(startIndex)
                {}

            private:
                friend class boost::iterator_core_access;
                void increment()
                {
                    ++currentIndex_;
                }

                void decrement()
                {
                    --currentIndex_;
                }

                bool equal(const const_iterator& other) const
                {
                    return (bufferArray_ == other.bufferArray_) && (currentIndex_ == other.currentIndex_);
                }

                void advance(const int n)
                {
                    currentIndex_ += n;
                }

                auto distance_to(const const_iterator& other) const
                {
                    if (bufferArray_ != other.bufferArray_)
                    {
                        return 0;
                    }
                    return other.currentIndex_ - currentIndex_;
                }

                value_type dereference() const
                {
                    if ((currentIndex_ < 0) || (bufferArray_->latestBuffer_ < 0) || (currentIndex_ > bufferArray_->latestBuffer_))
                    {
                        return {nullptr, 0};
                    }
                    if (currentIndex_ < bufferArray_->latestBuffer_)
                    {
                        return boost::asio::const_buffer(bufferArray_->buffers_[currentIndex_]->data, BufferSize);
                    }
                    return boost::asio::const_buffer(bufferArray_->buffers_[currentIndex_]->data,
                        bufferArray_->usedBytesInLatestBuffer_);
                }

                const ReadWriteBufferArray* bufferArray_;
                int currentIndex_;
            };

            const_iterator begin() const
            {
                return const_iterator(bufferArray_, 0);
            }

            const_iterator end() const
            {
                return const_iterator(bufferArray_, std::max(bufferArray_->latestBuffer_, 0) + 1);
            }

        private:
            const ReadWriteBufferArray* bufferArray_;
        };

        ConstBufferWrapper constBufferWrapper() const
        {
            return ConstBufferWrapper(this);
        }

    private:
        bool requestNewBuffer()
        {
            if (static_cast<size_t>(latestBuffer_ + 1) >= MaxNrOfBuffers)
            {
                return false;
            }
            auto buffer = bufferPool_.leaseBuffer();
            if ( ! buffer)
            {
                return false;
            }
            buffers_[++latestBuffer_] = std::move(buffer);
            usedBytesInLatestBuffer_ = 0;
            return true;
        }

        std::array<BufferType, MaxNrOfBuffers> buffers_;
        FixedSizeBufferPool<BufferSize>& bufferPool_;

        int latestBuffer_ = -1;
        size_t usedBytesInLatestBuffer_ = 0;
        bool notEnoughRoom_ = false;
    };
}
