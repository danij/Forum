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
        const Entities::IdType& getCurrentUserId();

        /**
         * Sets the id of the current user executing an action (thread-local)
         */
        void setCurrentUserId(const Entities::IdType& value);

        /**
         * Returns the IP address of the current user executing an action
         */
        const std::string& getCurrentUserIpAddress();

        /**
        * Sets the IP address of the current user executing an action (thread-local)
        */
        void setCurrentUserIpAddress(const std::string& value);

        /**
         * Returns whether validations should be skipped (e.g. for populating data in benchmarks)
         */
        bool skipValidations();

        /**
         * Returns whether observer action should be skipped (e.g. for populating data in benchmarks)
         */
        bool skipObservers();

        enum class SortOrder
        {
            Ascending,
            Descending
        };

        struct DisplayContext
        {
            SortOrder sortOrder = SortOrder::Ascending;
            int_fast32_t pageNumber = 0;
            Entities::Timestamp checkNotChangedSince = 0;
        };

        const DisplayContext& getDisplayContext();
        DisplayContext& getMutableDisplayContext();
    }
}