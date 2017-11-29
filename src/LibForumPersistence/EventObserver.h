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

#include "Observers.h"

#include <ctime>
#include <boost/filesystem.hpp>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Persistence
    {
        class EventObserver final : private boost::noncopyable
        {
        public:
            EventObserver(Repository::ReadEvents& readEvents, Repository::WriteEvents& writeEvents,
                          const boost::filesystem::path& destinationFolder, time_t refreshEverySeconds);
            ~EventObserver();

        private:
            struct EventObserverImpl;
            EventObserverImpl* impl_;
        };
    }
}
