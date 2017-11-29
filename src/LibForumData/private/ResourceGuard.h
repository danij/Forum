/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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
                const T& constResource = *resource_;
                action(constResource);
            }

            template<typename TAction>
            void write(TAction&& action) const /* lock is taken anyway so always allow access */
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
