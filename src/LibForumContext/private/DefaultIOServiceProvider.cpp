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

#include "DefaultIOServiceProvider.h"

#include <boost/asio/signal_set.hpp>

#include <algorithm>

using namespace Forum::Network;

DefaultIOServiceProvider::DefaultIOServiceProvider(size_t nrOfThreads) : signalWaiter_{service_, SIGINT, SIGTERM}
{
    nrOfThreads = std::clamp(nrOfThreads, size_t(1), size_t(100));

    threads_.reserve(nrOfThreads);
}

boost::asio::io_service& DefaultIOServiceProvider::getIOService()
{
    return service_;
}

void DefaultIOServiceProvider::start()
{
    for (decltype(threads_.capacity()) i = 0; i < threads_.capacity(); ++i)
    {
        threads_.emplace_back([&]
        {
            service_.run();
        });
    }
    signalWaiter_.async_wait([this](auto ec, auto /*signal*/)
    {
        if ( ! ec)
        {
            this->stop();            
        }
    });
}

void DefaultIOServiceProvider::waitForStop()
{
    while ( ! stop_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    service_.stop();

    if ( ! threads_.empty())
    {
        for (auto& thread : threads_)
        {
            thread.join();
        }
        threads_.clear();
    }
}

void DefaultIOServiceProvider::stop()
{
    stop_ = true;
}
