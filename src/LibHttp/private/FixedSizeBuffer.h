#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>

#include <boost/noncopyable.hpp>

namespace Http
{
    template<size_t BufferSize>
    class FixedSizeBufferManager final : boost::noncopyable
    {
    public:
        struct Buffer
        {
            std::array<char, BufferSize> data;
        };

        struct BringBackBuffer final
        {
            void operator()(Buffer* toReturn) 
            {
                if (manager) manager->returnBuffer(toReturn);
            }

            FixedSizeBufferManager* manager;
        };

        typedef std::unique_ptr<Buffer, BringBackBuffer> LeasedBufferType;

        explicit FixedSizeBufferManager(size_t maxBufferCount) :
            maxBufferCount_(maxBufferCount), numberOfUsedBuffers(0),
            buffers_(std::make_unique<Buffer[]>(maxBufferCount)),
            availableIndexes_(std::make_unique<size_t[]>(maxBufferCount))
        {
            for (size_t i = 0; i < maxBufferCount_; ++i)
            {
                availableIndexes_[i] = i;
            }
        }
        
        LeasedBufferType leaseBuffer()
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            if (numberOfUsedBuffers >= maxBufferCount_)
            {
                return {};
            }
            return{ &buffers_[availableIndexes_[numberOfUsedBuffers++]], { this } };
        }

    private:
        void returnBuffer(Buffer* value)
        {
            size_t index = value - &buffers_[0];
            if (index >= maxBufferCount_)
            {
                return;
            }

            std::lock_guard<decltype(mutex_)> lock(mutex_);
            if (numberOfUsedBuffers < 1)
            {
                return;
            }
            availableIndexes_[--numberOfUsedBuffers] = index;
        }

        const size_t maxBufferCount_;
        size_t numberOfUsedBuffers;
        std::unique_ptr<Buffer[]> buffers_;
        std::unique_ptr<size_t[]> availableIndexes_;
        std::mutex mutex_;
    };

    template<size_t BufferSize, size_t MaxNrOfBuffers>
    class ReadWriteBufferArray final : private boost::noncopyable
    {
    public:
        explicit ReadWriteBufferArray(FixedSizeBufferManager<BufferSize>& bufferManager) : bufferManager_(bufferManager)
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
                    std::copy(input, input + size, buffers_[latestBuffer_]->data.data() + usedBytesInLatestBuffer_);
                    usedBytesInLatestBuffer_ += size;
                    return true;
                }
                if ((latestBuffer_ < 0) || (remainingSpaceInCurrentBuffer < 1))
                {
                    //need a new buffer
                    if ( ! requestNewBuffer())
                    {
                        //no more room available
                        return false;
                    }
                    remainingSpaceInCurrentBuffer = BufferSize;
                }
                auto toCopy = std::min(remainingSpaceInCurrentBuffer, size);
                std::copy(input, input + toCopy, buffers_[latestBuffer_]->data.data() + usedBytesInLatestBuffer_);
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

        /**
         * Reads the data in the buffers by invoking a callback for each individual buffer.
         * The callback must accept a buffer and its size
         */
        template<typename Fn>        
        void readAllData(Fn&& callback)
        {
            if (latestBuffer_ < 0)
            {
                //nothing written
                return;
            }
            for (int i = 0; i < latestBuffer_; ++i)
            {
                //full buffers
                callback(buffers_[i]->data.data(), BufferSize);
            }
            //latest possible partial full buffer
            callback(buffers_[latestBuffer_]->data.data(), usedBytesInLatestBuffer_);
        }

    private:
        bool requestNewBuffer()
        {
            if ((latestBuffer_ + 1) >= MaxNrOfBuffers)
            {
                return false;
            }
            auto buffer = bufferManager_.leaseBuffer();
            if ( ! buffer)
            {
                return false;
            }
            buffers_[++latestBuffer_] = std::move(buffer);
            usedBytesInLatestBuffer_ = 0;
            return true;
        }

        typename FixedSizeBufferManager<BufferSize>::LeasedBufferType buffers_[MaxNrOfBuffers];
        FixedSizeBufferManager<BufferSize>& bufferManager_;

        int latestBuffer_ = -1;
        size_t usedBytesInLatestBuffer_ = 0;
    };
}
