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

#include <atomic>
#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <string_view>
#include <thread>
#include <cstring>

#include <boost/lockfree/queue.hpp>
#include <boost/noncopyable.hpp>

namespace Forum::Helpers
{
    template<typename Derived, typename T, uint16_t Capacity = 32768>
    class SeparateThreadConsumer : boost::noncopyable
    {
    public:
        explicit SeparateThreadConsumer(const std::chrono::milliseconds loopWaitMilliseconds)
            : loopWaitMilliseconds_(loopWaitMilliseconds), writeThread_{ [this]() { this->threadLoop(); } }
        {}

        virtual ~SeparateThreadConsumer()
        {
            stopConsumer();
        }

        /**
         * Can be called from any thread
         */
        void enqueue(T value)
        {
            uint32_t failNr = 0;
            while ( ! queue_.bounded_push(value))
            {
                static_cast<Derived*>(this)->onFail(failNr);
            }
            blobInQueueCondition_.notify_one();
        }

        void stopConsumer()
        {
            stopWriteThread_ = true;
            blobInQueueCondition_.notify_one();
            if (writeThread_.joinable())
            {
                writeThread_.join();                
            }
        }

    private:
        void threadLoop()
        {
            while ( ! stopWriteThread_)
            {
                std::unique_lock<decltype(conditionMutex_)> lock(conditionMutex_);
                if (blobInQueueCondition_.wait_for(lock, loopWaitMilliseconds_, [this]()
                {
                    return ! queue_.empty() || stopWriteThread_;
                }))
                {
                    consumeValues();                    
                }
                else
                {
                    static_cast<Derived*>(this)->onThreadWaitNoValues();
                }
            }
            consumeValues();

            static_cast<Derived*>(this)->onThreadFinish();
        }

        void consumeValues()
        {
            T values[Capacity];
            size_t nrOfValues = 0;

            queue_.consume_all([&values, &nrOfValues](T value)
            {
                values[nrOfValues++] = value;
            });

            if (nrOfValues > 0)
            {
                static_cast<Derived*>(this)->consumeValues(values, nrOfValues);
            }
        }

        boost::lockfree::queue<T, boost::lockfree::capacity<Capacity>> queue_;
        std::atomic_bool stopWriteThread_{ false };
        std::condition_variable blobInQueueCondition_;
        std::mutex conditionMutex_;
        std::chrono::milliseconds loopWaitMilliseconds_;
        //other variabiles need to be initialized once the thread starts
        std::thread writeThread_;        
    };

    struct SeparateThreadConsumerBlob final
    {
        char* buffer{ nullptr }; //storing raw pointer so that Blob can be placed in a boost lockfree queue
        size_t size{};

        static SeparateThreadConsumerBlob allocateNew(const size_t size)
        {
            return { new char[size], size };
        }

        static SeparateThreadConsumerBlob allocateCopy(const std::string_view view)
        {
            if (view.empty())
            {
                return { nullptr, view.size() };
            }
            auto buffer = new char[view.size()];
            memcpy(buffer, view.data(), view.size());

            return { buffer, view.size() };
        }

        static void free(SeparateThreadConsumerBlob& blob)
        {
            delete[] blob.buffer;
            blob.buffer = nullptr;
        }
    };
}