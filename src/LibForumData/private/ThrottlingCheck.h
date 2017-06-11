#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Authorization
    {
        template<typename TPeriod>
        class ThrottlingCheck final : boost::noncopyable
        {
        public:
            ThrottlingCheck(size_t maxAllowed, TPeriod period)
                : maxAllowed_(maxAllowed), period_(period), currentIndex_(0)
            {
                maxAllowed = std::max(maxAllowed, static_cast<decltype(maxAllowed)>(1));
                entries_.reset(new TPeriod[maxAllowed]());
                lockFlag_ = ATOMIC_FLAG_INIT;
            }

            bool isAllowed(TPeriod at)
            {
                //use a spin lock instead of a mutex for better performance,
                //as the function has little work to do
                while (lockFlag_.test_and_set(std::memory_order_acquire)){}

                bool result = false;
                if ((entries_.get()[currentIndex_] + period_) < at)
                {
                    result = true;
                }
                currentIndex_ += 1;
                while (currentIndex_ >= maxAllowed_)
                {
                    currentIndex_ -= maxAllowed_;
                }

                lockFlag_.clear(std::memory_order_release);
                return result;
            }

        private:
            size_t maxAllowed_;
            TPeriod period_;
            std::unique_ptr<TPeriod[]> entries_;
            size_t currentIndex_;
            std::atomic_flag lockFlag_;
        };
    }
}
