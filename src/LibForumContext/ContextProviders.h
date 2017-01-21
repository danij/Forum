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

        /**
         * Returns the IP address of the current user executing an action
         */
        const std::string& getCurrentUserIpAddress();

        /**
        * Sets the IP address of the current user executing an action (thread-local)
        */
        void setCurrentUserIpAddress(const std::string& value);

        /**
        * Returns the browser user agent of the current user executing an action
        */
        const std::string& getCurrentUserBrowserUserAgent();

        /**
        * Sets the browser user agent of the current user executing an action (thread-local)
        */
        void setCurrentUserBrowserUserAgent(const std::string& value);


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