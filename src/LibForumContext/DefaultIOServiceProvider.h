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

#include "IOServiceProvider.h"

#include <boost/asio/signal_set.hpp>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace Forum::Network
{
    class DefaultIOServiceProvider final : public IIOServiceProvider, boost::noncopyable
    {
    public:
        explicit DefaultIOServiceProvider(size_t nrOfThreads);

        boost::asio::io_service& getIOService() override;
        void start() override;
        void waitForStop() override;
        void stop() override;

    private:
        std::atomic_bool stop_{ false };
        boost::asio::io_service service_;

        std::vector<std::thread> threads_;
        boost::asio::signal_set signalWaiter_;
    };
}
