#pragma once

#include "HttpListener.h"
#include "HttpRouter.h"
#include "CommandHandler.h"
#include "ServiceEndpointManager.h"
#include "EventObserver.h"

#include <boost/noncopyable.hpp>

#include <memory>
#include <string>

namespace Forum
{
    class Application final : boost::noncopyable
    {
    public:
        int run(int argc, const char* argv[]);

    private:
        void cleanup();
        bool initialize(const std::string& configurationFileName);
        void validateConfiguration();
        void createCommandHandler();
        void importEvents();
        void initializeHttp();
        void initializeLogging();

        std::unique_ptr<Http::HttpRouter> httpRouter_;
        std::unique_ptr<Http::HttpListener> httpListener_;
        std::unique_ptr<Commands::CommandHandler> commandHandler_;
        std::unique_ptr<Commands::ServiceEndpointManager> endpointManager_;
        std::unique_ptr<Persistence::EventObserver> persistenceObserver_;

        Entities::EntityCollectionRef entityCollection_;
        Repository::DirectWriteRepositoryCollection directWriteRepositories_;
    };
}
