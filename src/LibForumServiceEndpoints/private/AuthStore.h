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

#pragma once

#include "ContextProviders.h"
#include "EntityCommonTypes.h"
#include "ResourceGuard.h"

#include <atomic>
#include <unordered_map>
#include <string>
#include <tuple>

namespace Forum::Commands
{
    /**
     * Thread-safe collection for mappings between auth tokens and auth ids
     */
    class AuthStore
    {
    public:
        void add(const std::string& authToken, const std::string& authId, const Entities::Timestamp expiresIn)
        {
            cleanup();
            auto expiresAt = Context::getCurrentTime() + expiresIn;

            mapGuard_.write([&](MapType& collection)
            {
                collection.insert(std::make_pair(authToken, std::make_tuple(authId, expiresAt)));
            });
        }

        //returns a copy of the string as the value might be removed at any time
        std::string find(const std::string& authToken) const
        {
            std::string result;

            mapGuard_.read([&](const MapType& collection)
            {
                const auto it = collection.find(authToken);
                if (it == collection.end()) return;

                auto&[authId, expiresAt] = it->second;
                if (expiresAt < Context::getCurrentTime()) return;

                result = authId;
            });

            return result;
        }

        void cleanup()
        {
            constexpr Entities::Timestamp cleanupEverySeconds = 30;

            const auto now = Context::getCurrentTime();
            if ((now - lastCleanup_) < cleanupEverySeconds) return;

            mapGuard_.write([now](MapType& collection)
            {
               for (auto it = collection.begin(); it != collection.end();)
               {
                   auto&[authId, expiresAt] = it->second;
                   (void)authId;
                   if (expiresAt < now)
                   {
                       it = collection.erase(it);
                   }
                   else
                   {
                       ++it;
                   }
               }
            });

            lastCleanup_ = now;
        }

    private:
        using MapType = std::unordered_map<std::string, std::tuple<std::string, Entities::Timestamp>>;
        
        Helpers::ResourceGuard<MapType> mapGuard_{ std::make_shared<MapType>() };
        std::atomic<Entities::Timestamp> lastCleanup_{};
    };
}
