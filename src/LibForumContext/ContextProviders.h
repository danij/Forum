#pragma once

#include "EntityCommonTypes.h"

namespace Forum
{
    namespace Context
    {
        /**
         * Returns the current time as the number of seconds elapsed since the UNIX EPOCH
         */
        Forum::Entities::Timestamp getCurrentTime();
    }
}