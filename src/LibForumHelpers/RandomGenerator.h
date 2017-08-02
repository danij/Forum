#pragma once

#include "UuidString.h"
#include "EntityCommonTypes.h"

namespace Forum
{
    namespace Helpers
    {
        boost::uuids::uuid generateUUID();
        Entities::IdType generateUniqueId();
    }
}
