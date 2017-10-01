#pragma once

#include <atomic>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Helpers
    {
        class SpinLock final : private boost::noncopyable
        {
        public:
            void lock()
            {
                while (lockFlag_.test_and_set(std::memory_order_acquire)) {}
            }
            void unlock()
            {
                lockFlag_.clear(std::memory_order_release);
            }
        private:
            std::atomic_flag lockFlag_ = ATOMIC_FLAG_INIT;
        };
    }
}
