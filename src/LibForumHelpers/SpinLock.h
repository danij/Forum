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

#include <atomic>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Helpers
    {
        class SpinLock final : private boost::noncopyable
        {
        public:
            void lock()
            {
                while (lockFlag_.test_and_set(std::memory_order_acq_rel)) {}
            }

            void unlock()
            {
                lockFlag_.clear(std::memory_order_release);
            }
        private:
            std::atomic_flag lockFlag_ = ATOMIC_FLAG_INIT;
        };
    }
}
