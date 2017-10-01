#pragma once

#include "SpinLock.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>

namespace Forum
{
    namespace Authorization
    {
        template<typename TPeriod>
        class ThrottlingCheck final
        {
        public:
            typedef uint16_t IndexType;

            ThrottlingCheck() : maxAllowed_(0), period_{}, currentIndex_(0)
            {
            }

            ThrottlingCheck(IndexType maxAllowed, TPeriod period)
                : maxAllowed_(maxAllowed), period_(period), currentIndex_(0)
            {
                maxAllowed = std::max(maxAllowed, static_cast<IndexType>(1u));
                entries_.reset(new TPeriod[maxAllowed]());
            }

            ThrottlingCheck(const ThrottlingCheck&) = delete;
            ThrottlingCheck(ThrottlingCheck&& other) noexcept
            {
                swap(*this, other);
            }

            ThrottlingCheck& operator=(const ThrottlingCheck&) = delete;
            ThrottlingCheck& operator=(ThrottlingCheck&& other) noexcept
            {
                swap(*this, other);
                return *this;
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

            friend void swap(ThrottlingCheck& first, ThrottlingCheck& second) noexcept
            {
                std::lock_guard<decltype(spinLock_)> lock1(first.spinLock_);
                std::lock_guard<decltype(spinLock_)> lock2(second.spinLock_);

                std::swap(first.maxAllowed_, second.maxAllowed_);
                std::swap(first.period_, second.period_);
                std::swap(first.entries_, second.entries_);
                std::swap(first.currentIndex_, second.currentIndex_);
            }

        private:
            IndexType maxAllowed_;
            TPeriod period_;
            std::unique_ptr<TPeriod[]> entries_;
            IndexType currentIndex_;
            //use a spin lock instead of a mutex for better performance,
            //as the function has little work to do
            Helpers::SpinLock spinLock_;
        };
    }
}
