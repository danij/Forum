#pragma once

#include <string>

#include "UuidString.h"

namespace Forum
{
    namespace Repository
    {
        boost::uuids::uuid generateUUID();
        Forum::Entities::UuidString generateUUIDString();
    }
}