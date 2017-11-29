/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "EntityCommonTypes.h"
#include "TypeHelpers.h"
#include "IpAddress.h"

#include <boost/asio/io_service.hpp>
#include <boost/signals2/signal.hpp>

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
        Entities::IdTypeRef getCurrentUserId();

        /**
         * Sets the id of the current user executing an action (thread-local)
         */
        void setCurrentUserId(Entities::IdType value);

        /**
         * Returns the IP address of the current user executing an action
         */
        const Helpers::IpAddress& getCurrentUserIpAddress();

        /**
        * Sets the IP address of the current user executing an action (thread-local)
        */
        void setCurrentUserIpAddress(Helpers::IpAddress value);

        /**
         * Returns whether a batch insert is currently in progress for optimization purposes
         */
        bool isBatchInsertInProgress();

        /**
         * Sets whether a batch insertion of entities is about to start or has ended
         */
        void setBatchInsertInProgres(bool value);

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

        class IIOServiceProvider
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IIOServiceProvider)

            virtual boost::asio::io_service& getIOService() = 0;
            virtual void start() = 0;
            virtual void waitForStop() = 0;
            virtual void stop() = 0;
        };

        IIOServiceProvider& getIOServiceProvider();
        void setIOServiceProvider(std::unique_ptr<IIOServiceProvider>&& provider);

        struct ApplicationEventCollection
        {
            boost::signals2::signal<void()> onApplicationStart;
            boost::signals2::signal<void()> beforeApplicationStop;
        };

        ApplicationEventCollection& getApplicationEvents();
        void setApplicationEventCollection(std::unique_ptr<ApplicationEventCollection>&& collection);
    }
}
