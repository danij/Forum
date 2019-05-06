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

#include "ConnectionManagerWithTimeout.h"
#include "DefaultIOServiceProvider.h"
#include "FixedHttpConnectionManager.h"
#include "HttpRouter.h"
#include "TcpListener.h"

#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <boost/noncopyable.hpp>

using namespace Forum::Network;
using namespace Http;

class Endpoints final : boost::noncopyable
{
public:
    void registerRoutes(HttpRouter& router)
    {
        router.addRoute("hello", HttpVerb::GET, [this](auto& state) { this->hello(state); });
        router.addRoute("count", HttpVerb::GET, [this](auto& state) { this->count(state); });
    }

private:
    void hello(RequestState& requestState)
    {
        auto& response = requestState.response;

        response.writeResponseCode(requestState.request, HttpStatusCode::OK);
        response.writeBodyAndContentLength("Hello World");
    }

    std::atomic_uint64_t counter_{};
    void count(RequestState& requestState)
    {
        auto& response = requestState.response;
        const auto text = std::string("Current count: ") + std::to_string(++counter_);
        
        response.writeResponseCode(requestState.request, HttpStatusCode::OK);
        response.writeBodyAndContentLength(text);
    }
};

class Application final : boost::noncopyable
{
public:
    int run(int /*argc*/, const char* /*argv*/[])
    {
        try
        {
            initialize();
        }
        catch (std::exception& ex)
        {
            std::cerr << "Could not initialize: " << ex.what() << '\n';
            return 1;
        }

        try
        {
            tcpListener_->startListening();
        }
        catch (std::exception& ex)
        {
            std::cerr << "Could not start listening: " << ex.what() << '\n';
            return 1;
        }

        ioServiceProvider_->start();
        ioServiceProvider_->waitForStop();

        tcpListener_->stopListening();

        return 0;
    }

private:
    void initialize()
    {
        ioServiceProvider_ = std::make_unique<DefaultIOServiceProvider>(std::thread::hardware_concurrency());
        auto& ioService = ioServiceProvider_->getIOService();
        
        endpoints_ = std::make_unique<Endpoints>();

        httpRouter_ = std::make_unique<HttpRouter>();
        endpoints_->registerRoutes(*httpRouter_);

        auto httpConnectionManager = std::make_shared<FixedHttpConnectionManager>(ioService,
                std::move(httpRouter_), 10, 10, 10, false);

        auto connectionManagerWithTimeout = std::make_shared<ConnectionManagerWithTimeout>(ioService,
            httpConnectionManager, 30);

        tcpListener_ = std::make_unique<TcpListener>(ioService, "127.0.0.1", 8081, connectionManagerWithTimeout);
    }

    std::unique_ptr<DefaultIOServiceProvider> ioServiceProvider_;
    std::unique_ptr<HttpRouter> httpRouter_;
    std::unique_ptr<TcpListener> tcpListener_;
    std::unique_ptr<Endpoints> endpoints_;
};

int main(int argc, const char* argv[])
{
    Application app;

    return app.run(argc, argv);
}