#pragma once

#include "CommandHandler.h"
#include "HttpRouter.h"

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Commands
    {
        class ServiceEndpointManager final : private boost::noncopyable
        {
        public:
            explicit ServiceEndpointManager(CommandHandler& handler);
            ~ServiceEndpointManager();

            void registerRoutes(Http::HttpRouter& router);

        private:
            struct ServiceEndpointManagerImpl;
            ServiceEndpointManagerImpl* impl_;
        };
    }
}
