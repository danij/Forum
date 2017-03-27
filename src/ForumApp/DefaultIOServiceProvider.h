#pragma once

#include "ContextProviders.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace Forum
{
    namespace Network
    {
        class DefaultIOServiceProvider final : public Context::IIOServiceProvider, private boost::noncopyable
        {
        public:
            DefaultIOServiceProvider();

            boost::asio::io_service& getIOService() override;
            void waitForStop() override;
            void stop() override;

        private:
            boost::asio::io_service service_;

            std::vector<std::thread> threads_;
            std::condition_variable stopVariable_;
            std::mutex stopMutex_;
            bool stopping_;
        };
    }
}
