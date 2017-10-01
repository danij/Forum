#pragma once

#include "SpinLock.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>

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
            }

            bool isAllowed(TPeriod at)
            {
                std::lock_guard<decltype(spinLock_)> lock(spinLock_);
                bool result = false;

                auto& oldestEntry = entries_.get()[currentIndex_];
                if ((oldestEntry + period_) < at)
                {
                    result = true;
                }
                //the oldest entry now becomes the newest one
                oldestEntry = at;

                currentIndex_ += 1;
                while (currentIndex_ >= maxAllowed_)
                {
                    currentIndex_ -= maxAllowed_;
                }

                return result;
            }

        private:
            size_t maxAllowed_;
            TPeriod period_;
            std::unique_ptr<TPeriod[]> entries_;
            size_t currentIndex_;
            //use a spin lock instead of a mutex for better performance,
            //as the function has little work to do
            Helpers::SpinLock spinLock_;
        };
    }
}
