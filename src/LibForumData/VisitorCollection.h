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

#include "EntityCommonTypes.h"

#include <boost/noncopyable.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace Forum::Repository
{
    /**
    * Thread-safe collection for mappings between auth tokens and auth ids
    */
    class VisitorCollection : boost::noncopyable
    {
    public:
        using VisitorId = uint64_t;

        explicit VisitorCollection(const Entities::Timestamp visitForSeconds) : visitForSeconds_{visitForSeconds}
        {            
        }

        uint64_t currentNumberOfVisitors() const { return currentNumberOfVisitors_; }

        void add(VisitorId visitor);

        void cleanup();

    private:
        const Entities::Timestamp visitForSeconds_{};

        std::unordered_map<VisitorId, Entities::Timestamp> collection_;
        std::mutex mutex_;

        std::atomic<Entities::Timestamp> lastCleanup_{};
        std::atomic<uint64_t> currentNumberOfVisitors_{};
    };
}
