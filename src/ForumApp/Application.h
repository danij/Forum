#pragma once

#include "HttpListener.h"
#include "HttpRouter.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace Forum
{
    class Application final : boost::noncopyable
    {
    public:
        Application(int argc, const char* argv[]);
        int run();

    private:
        void cleanup();

        std::unique_ptr<Http::HttpRouter> httpRouter_;
        std::unique_ptr<Http::HttpListener> httpListener_;
    };
}
