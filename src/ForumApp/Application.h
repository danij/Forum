#pragma once

#include <boost/noncopyable.hpp>

namespace Forum
{
    class Application final : boost::noncopyable
    {
    public:
        Application(int argc, const char* argv[]);
        int run();

    private:
        void cleanup();
    };
}
