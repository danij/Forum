#pragma once

#include <boost/uuid/uuid.hpp>

namespace Forum
{
    namespace Repository
    {
        boost::uuids::uuid generateUUID();
    }
}