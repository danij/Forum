#pragma once

#include <cstddef>
#include <memory>
#include <mutex>

#include <boost/noncopyable.hpp>

namespace Http
{
    template<size_t BufferSize>
    class ConstantBufferManager final : boost::noncopyable
    {
    public:
        struct Buffer
        {
            std::array<char, BufferSize> data;
        };

        typedef std::unique_ptr<Buffer, std::function<void(Buffer*)>> LeasedBufferType;

        explicit ConstantBufferManager(size_t maxBufferCount) :
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
            return 
            {
                &buffers_[availableIndexes_[numberOfUsedBuffers++]],
                [this](auto buffer) { this->returnBuffer(buffer); } 
            };
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
}
