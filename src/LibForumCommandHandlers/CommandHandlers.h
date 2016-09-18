#pragma once

#include <iosfwd>
#include <vector>

namespace Forum
{
    namespace Commands
    {
        void version(const std::vector<std::string>& parameters, std::ostream& output);
    }
}