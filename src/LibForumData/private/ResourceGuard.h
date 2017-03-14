#pragma once

#include <boost/core/noncopyable.hpp>

#include <memory>
#include <mutex>
#include <shared_mutex>

namespace Forum
{
    namespace Helpers
    {
        template <typename T>
        class ResourceGuard final : private boost::noncopyable
        {
        public:
            explicit ResourceGuard(std::shared_ptr<T> resource) : resource_(std::move(resource)) {}

            template<typename TAction>
            void read(TAction&& action) const
            {
                std::shared_lock<decltype(mutex_)> lock(mutex_);
                action(const_cast<const T&>(*resource_));
            }

            template<typename TAction>
            void write(TAction&& action)
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
