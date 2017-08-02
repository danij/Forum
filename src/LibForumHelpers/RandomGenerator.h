#pragma once

#include "UuidString.h"

namespace Forum
{
    namespace Helpers
    {
        boost::uuids::uuid generateUUID();
        Entities::UuidString generateUniqueId();
    }
}
