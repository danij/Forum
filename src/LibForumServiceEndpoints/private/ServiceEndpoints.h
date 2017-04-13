#pragma once

#include "CommandHandler.h"
#include "HttpRouter.h"

#include <vector>

namespace Forum
{
    namespace Commands
    {
        //Endpoints can be called from multiple threads

        class AbstractEndpoint
        {
        public:
            explicit AbstractEndpoint(CommandHandler& handler);

        protected:
            void handleDefault(Http::RequestState& requestState, View view);

            CommandHandler& commandHandler_;
        };

        class MetricsEndpoint : private AbstractEndpoint
        {
        public:
            explicit MetricsEndpoint(CommandHandler& handler);

            void getVersion(Http::RequestState& requestState);
        };

        class StatisticsEndpoint : private AbstractEndpoint
        {
        public:
            explicit StatisticsEndpoint(CommandHandler& handler);

            void getEntitiesCount(Http::RequestState& requestState);
        };

    }
}
