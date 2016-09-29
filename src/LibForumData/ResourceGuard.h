#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>

#include <boost/core/noncopyable.hpp>

namespace Forum
{
    namespace Helpers
    {
        template <typename T>
        class ResourceGuard final : private boost::noncopyable
        {
        public:
            ResourceGuard(std::shared_ptr<T> resource) : resource_(resource) {}

            inline void read(std::function<void(const T&)> action) const
            {
                std::shared_lock<decltype(mutex_)> lock(mutex_);
                action(*resource_);
            }

            inline void write(std::function<void(T&)> action)
            {
                std::unique_lock<decltype(mutex_)> lock(mutex_);
                action(*resource_);
            }

        private:
            std::shared_ptr<T> resource_;
            mutable std::shared_timed_mutex mutex_;
        };
    }
}
