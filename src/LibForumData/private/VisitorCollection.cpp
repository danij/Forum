/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "VisitorCollection.h"
#include "ContextProviders.h"

using namespace Forum::Entities;
using namespace Forum::Repository;

void VisitorCollection::add(VisitorId visitor)
{
    cleanup();
    auto expiresAt = Context::getCurrentTime() + visitForSeconds_;

    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);

        auto [it, result] = collection_.insert(std::make_pair(visitor, expiresAt));
        if (result)
        {
            currentNumberOfVisitors_ += 1;
        }
        else
        {
            it->second = expiresAt;
        }
    }
}

void VisitorCollection::cleanup()
{
    constexpr Timestamp cleanupEverySeconds = 30;

    const auto now = Context::getCurrentTime();
    if ((now - lastCleanup_) < cleanupEverySeconds) return;

    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);

        for (auto it = collection_.begin(); it != collection_.end();)
        {
            const auto expiresAt = it->second;
            if (expiresAt < now)
            {
                it = collection_.erase(it);
                currentNumberOfVisitors_ -= 1;
            }
            else
            {
                ++it;
            }
        }
    }

    lastCleanup_ = now;
}
