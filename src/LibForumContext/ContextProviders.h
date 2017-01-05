#pragma once

#include "EntityCommonTypes.h"

namespace Forum
{
    namespace Context
    {
        /**
         * Returns the current time as the number of seconds elapsed since the UNIX EPOCH
         */
        Entities::Timestamp getCurrentTime();

        /**
         * Returns the id of the current user executing an action
         */
        Entities::IdType getCurrentUserId();

        /**
         * Sets the id of the current user executing an action (thread-local)
         */
        void setCurrentUserId(Entities::IdType value);

        enum class SortOrder
        {
            Ascending,
            Descending
        };

        struct DisplayContext
        {
            SortOrder sortOrder = SortOrder::Ascending;
            int_fast32_t pageNumber = 0;
            int_fast32_t pageSize = std::numeric_limits<decltype(pageSize)>::max();
        };

        const DisplayContext& getDisplayContext();
        DisplayContext& getMutableDisplayContext();
    }
}