#include "Application.h"
#include "Configuration.h"
#include "ContextProviders.h"
#include "DefaultIOServiceProvider.h"
#include "StringHelpers.h"

#include "Authorization.h"

#include <unicode/uclean.h>
#include "MemoryRepositoryCommon.h"
#include "MemoryRepositoryUser.h"
#include "MemoryRepositoryDiscussionThread.h"
#include "MemoryRepositoryDiscussionThreadMessage.h"
#include "MemoryRepositoryDiscussionTag.h"
#include "MemoryRepositoryDiscussionCategory.h"
#include "MemoryRepositoryStatistics.h"
#include "MetricsRepository.h"

#include "Version.h"

#include <fstream>
#include <iostream>

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/from_stream.hpp>

using namespace Forum;
using namespace Forum::Authorization;
using namespace Forum::Commands;
using namespace Forum::Context;
using namespace Forum::Network;
using namespace Forum::Repository;

using namespace Http;

Application::Application(int argc, const char* argv[])
{
    setApplicationEventCollection(std::make_unique<ApplicationEventCollection>());
    setIOServiceProvider(std::make_unique<DefaultIOServiceProvider>());

    initializeLogging();

    BOOST_LOG_TRIVIAL(info) << "Starting Forum Backend v" << VERSION;

    initializeHttp();
}

int Application::run()
{
    auto& events = getApplicationEvents();

    events.onApplicationStart();
    
    httpListener_->startListening();

    getIOServiceProvider().start();
    getIOServiceProvider().waitForStop();
    
    httpListener_->stopListening();

    BOOST_LOG_TRIVIAL(info) << "Stopped listening for HTTP connections";

    events.beforeApplicationStop();
    
    cleanup();

    return 0;
}

void Application::cleanup()
{
    Helpers::cleanupStringHelpers();

    //clean up resources cached by ICU so that they don't show up as memory leaks
    u_cleanup();
}

void Application::validateConfiguration()
{
    //TODO
}

void Application::createCommandHandler()
{
    auto authorization = std::make_shared<AllowAllAuthorization>();

    auto store = std::make_shared<MemoryStore>(std::make_shared<Entities::EntityCollection>());

    auto userRepository = std::make_shared<MemoryRepositoryUser>(store, authorization);
    auto discussionThreadRepository = std::make_shared<MemoryRepositoryDiscussionThread>(store, authorization);
    auto discussionThreadMessageRepository = std::make_shared<MemoryRepositoryDiscussionThreadMessage>(store, authorization);
    auto discussionTagRepository = std::make_shared<MemoryRepositoryDiscussionTag>(store, authorization);
    auto discussionCategoryRepository = std::make_shared<MemoryRepositoryDiscussionCategory>(store, authorization);
    auto statisticsRepository = std::make_shared<MemoryRepositoryStatistics>(store, authorization);
    auto metricsRepository = std::make_shared<MetricsRepository>(store, authorization);

    ObservableRepositoryRef observableRepository = userRepository;

    commandHandler_ = std::make_unique<Commands::CommandHandler>(observableRepository, 
                                                                 userRepository, 
                                                                 discussionThreadRepository,
                                                                 discussionThreadMessageRepository, 
                                                                 discussionTagRepository, 
                                                                 discussionCategoryRepository,
                                                                 statisticsRepository, 
                                                                 metricsRepository);
}

void Application::initializeHttp()
{
    auto forumConfig = Configuration::getGlobalConfig();
    createCommandHandler();

    //load data before starting to serve HTTP requests

    HttpListener::Configuration httpConfig;
    httpConfig.numberOfIOServiceThreads = forumConfig->service.numberOfIOServiceThreads;
    httpConfig.numberOfReadBuffers = forumConfig->service.numberOfReadBuffers;
    httpConfig.numberOfWriteBuffers = forumConfig->service.numberOfWriteBuffers;
    httpConfig.listenIPAddress = forumConfig->service.listenIPAddress;
    httpConfig.listenPort = forumConfig->service.listenPort;
    httpConfig.connectionTimeoutSeconds = forumConfig->service.connectionTimeoutSeconds;
    httpConfig.trustIpFromXForwardedFor = forumConfig->service.trustIpFromXForwardedFor;

    httpRouter_ = std::make_unique<HttpRouter>();
    endpointManager_ = std::make_unique<ServiceEndpointManager>(*commandHandler_);
    endpointManager_->registerRoutes(*httpRouter_);

    httpListener_ = std::make_unique<HttpListener>(httpConfig, *httpRouter_, getIOServiceProvider().getIOService());
}

void Application::initializeLogging()
{
    auto forumConfig = Configuration::getGlobalConfig();
    auto& settingsFile = forumConfig->logging.settingsFile;

    if (settingsFile.size() < 1) return;

    std::ifstream file(settingsFile);
    if ( ! file)
    {
        std::cerr << "Unable to find log settings file: " << settingsFile << '\n';
    }
    try
    {
        boost::log::init_from_stream(file);
    }
    catch(std::exception& ex)
    {
        std::cerr << "Unable to load log settings from file: " << ex.what() << '\n';
    }
}
