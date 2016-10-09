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

        /**
         * Returns the id of the current user executing an action
         */
        Forum::Entities::IdType getCurrentUserId();

        /**
         * Sets the id of the current user executing an action (thread-local)
         */
        void setCurrentUserId(Forum::Entities::IdType value);
    }
}