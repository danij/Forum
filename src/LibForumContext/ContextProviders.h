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
        void setCurrentUserId(Entities::IdTypeRef value);

        /**
         * Returns the IP address of the current user executing an action
         */
        const Helpers::IpAddress& getCurrentUserIpAddress();

        /**
        * Sets the IP address of the current user executing an action (thread-local)
        */
        void setCurrentUserIpAddress(const Helpers::IpAddress& value);

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
