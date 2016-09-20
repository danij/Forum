#pragma once

#include <string>
#include <sstream>
#include <vector>

namespace Forum
{
    namespace Helpers
    {
        template<typename HandlerFn>
        std::string handlerToString(HandlerFn fn, const std::vector<std::string>& parameters)
        {
            std::stringstream stream;
            fn(parameters, stream);
            return stream.str();
        }

        template<typename HandlerFn>
        std::string handlerToString(HandlerFn fn)
        {
            return handlerToString(fn, {});
        }
    }
}
