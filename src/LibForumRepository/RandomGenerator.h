#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

namespace Forum
{
    namespace Repository
    {
        boost::uuids::uuid generateUUID();
        std::string generateUUIDString();
    }
}