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

#include "DefaultIOServiceProvider.h"
#include "Configuration.h"

using namespace Forum::Network;

DefaultIOServiceProvider::DefaultIOServiceProvider() : stopping_(false)
{
    auto nrOfThreads = Configuration::getGlobalConfig()->service.numberOfIOServiceThreads;
    if (nrOfThreads < 1) nrOfThreads = 1;

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
}

void DefaultIOServiceProvider::waitForStop()
{
    std::unique_lock<decltype(stopMutex_)> lock(stopMutex_);
    stopVariable_.wait(lock, [&]() { return stopping_; });

    if (threads_.size())
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
    {
        std::lock_guard<decltype(stopMutex_)> guard(stopMutex_);
        stopping_ = true;
    }
    stopVariable_.notify_all();
}
