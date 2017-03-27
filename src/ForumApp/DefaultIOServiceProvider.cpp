#include "DefaultIOServiceProvider.h"
#include "Configuration.h"

using namespace Forum::Network;

DefaultIOServiceProvider::DefaultIOServiceProvider() : stopping_(false)
{
    auto nrOfThreads = Configuration::getGlobalConfig()->service.numberOfIOServiceThreads;
    if (nrOfThreads < 1) nrOfThreads = 1;

    for (decltype(nrOfThreads) i = 0; i < nrOfThreads; ++i)
    {
        threads_.emplace_back([&]() { service_.run(); });
    }
}

boost::asio::io_service& DefaultIOServiceProvider::getIOService()
{
    return service_;
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
