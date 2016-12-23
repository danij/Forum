#pragma once

#include "UuidString.h"

namespace Forum
{
    namespace Repository
    {
        boost::uuids::uuid generateUUID();
        Entities::UuidString generateUUIDString();
    }
}