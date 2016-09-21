#pragma once

#include "CommandHandler.h"

namespace Forum
{
    namespace Helpers
    {
        Forum::Commands::CommandHandlerRef createCommandHandler();

        std::string handlerToString(Forum::Commands::CommandHandlerRef handler, Forum::Commands::Command command,
                                    const std::vector<std::string>& parameters);
        std::string handlerToString(Forum::Commands::CommandHandlerRef handler, Forum::Commands::Command command);
    }
}
