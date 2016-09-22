#pragma once

#include <algorithm>
#include <string>

namespace Forum
{
    namespace Helpers
    {
        inline bool stringNullOrEmpty(const std::string& value)
        {
            return value.empty() || std::all_of(value.begin(), value.end(), [](auto c) { return std::isspace(c) != 0; });
        }
    }
}
