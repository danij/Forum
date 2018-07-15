/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "Application.h"
#include "Configuration.h"
#include "ContextProviders.h"
#include "DefaultIOServiceProvider.h"
#include "StringHelpers.h"

#include "DefaultAuthorization.h"

#include "MemoryRepositoryCommon.h"
#include "MemoryRepositoryUser.h"
#include "MemoryRepositoryDiscussionThread.h"
#include "MemoryRepositoryDiscussionThreadMessage.h"
#include "MemoryRepositoryDiscussionTag.h"
#include "MemoryRepositoryDiscussionCategory.h"
#include "MemoryRepositoryAuthorization.h"
#include "MemoryRepositoryStatistics.h"
#include "MetricsRepository.h"

#include "Logging.h"
#include "EventImporter.h"
#include "Version.h"

#include <unicode/uclean.h>

#include <fstream>
#include <iostream>
#include <string>

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_stream.hpp>

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

using namespace Forum;
using namespace Forum::Authorization;
using namespace Forum::Commands;
using namespace Forum::Context;
using namespace Forum::Network;
using namespace Forum::Persistence;
using namespace Forum::Repository;

using namespace Http;

bool Application::initialize()
{
    setApplicationEventCollection(std::make_unique<ApplicationEventCollection>());
    setIOServiceProvider(std::make_unique<DefaultIOServiceProvider>(
            Configuration::getGlobalConfig()->service.numberOfIOServiceThreads));

    initializeLogging();

    FORUM_LOG_INFO << "Starting Forum Backend v" << VERSION;

    createCommandHandler();
    FORUM_LOG_INFO << "Initialized command handlers";

    FORUM_LOG_INFO << "Starting import of persisted events";
    importEvents();

    initializeHttp();

    return true;
}

int Application::run(int argc, const char* argv[])
{
    boost::program_options::options_description options("Available options");
    options.add_options()
            ("help", "Display available options")
            ("version", "Display the current version")
            ("config,c", boost::program_options::value<std::string>(), "Specify the location of the configuration file");

    boost::program_options::variables_map arguments;

    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), arguments);
        boost::program_options::notify(arguments);
    }
    catch (std::exception& ex)
    {
        std::cerr << "Invalid command line: " << ex.what() << '\n';
        return 1;
    }

    if (arguments.count("help"))
    {
        std::cout << options << '\n';
        return 1;
    }

    if (arguments.count("version"))
    {
        std::cout << "Forum Backend v" << VERSION << '\n';
        return 1;
    }

    if (arguments.count("config") < 1)
    {
        std::cerr << "Specifying a configuration file is required for starting the service\n";
        return 1;
    }

    const auto configFileName = arguments["config"].as<std::string>();

    if ( ! loadConfiguration(configFileName))
    {
        return 1;
    }

    if ( ! initialize())
    {
        std::cerr << "Initialization failed!\n";
        return 1;
    }

    auto& events = getApplicationEvents();

    events.onApplicationStart();

    const auto config = Configuration::getGlobalConfig();

    FORUM_LOG_INFO << "Starting to listen under "
                   << config->service.listenIPAddress << ":" << config->service.listenPort;
    try
    {
        httpListener_->startListening();
    }
    catch(std::exception& ex)
    {
        FORUM_LOG_ERROR << "Could not start listening: " << ex.what();
        std::cerr << "Could not start listening: " << ex.what() << '\n';
        return 1;
    }

    getIOServiceProvider().start();
    getIOServiceProvider().waitForStop();

    httpListener_->stopListening();

    FORUM_LOG_INFO << "Stopped listening for HTTP connections";

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

bool Application::loadConfiguration(const std::string_view fileName)
{
    try
    {
        if ( ! std::filesystem::exists(fileName))
        {
            std::cerr << "The configuration file '" << fileName << "' does not exist!\n";
            return false;
        }

        if ( ! std::filesystem::is_regular_file(fileName))
        {
            std::cerr << "The configuration file '" << fileName << "' is not a regular file!\n";
            return false;
        }

        std::ifstream input(fileName);
        Configuration::loadGlobalConfigFromStream(input);

        return true;
    }
    catch (std::exception& ex)
    {
        std::cerr << "Error loading configuration: " << ex.what() << '\n';
        return false;
    }
}

void Application::validateConfiguration()
{
    //TODO
}

void Application::createCommandHandler()
{
    const auto config = Configuration::getGlobalConfig();
    entityCollection_ = std::make_shared<Entities::EntityCollection>(config->persistence.messagesFile);

    auto store = std::make_shared<MemoryStore>(entityCollection_);
    auto authorization = std::make_shared<DefaultAuthorization>(entityCollection_->grantedPrivileges(),
                                                                *entityCollection_, config->service.disableThrottling);

    auto authorizationRepository = std::make_shared<MemoryRepositoryAuthorization>(
            store, authorization, authorization, authorization, authorization, authorization);

    auto userRepository = std::make_shared<MemoryRepositoryUser>(store, authorization, authorizationRepository);
    auto discussionThreadRepository = std::make_shared<MemoryRepositoryDiscussionThread>(store, authorization,
                                                                                         authorizationRepository);
    auto discussionThreadMessageRepository = std::make_shared<MemoryRepositoryDiscussionThreadMessage>(store, authorization);
    auto discussionTagRepository = std::make_shared<MemoryRepositoryDiscussionTag>(store, authorization);
    auto discussionCategoryRepository = std::make_shared<MemoryRepositoryDiscussionCategory>(store, authorization);
    auto statisticsRepository = std::make_shared<MemoryRepositoryStatistics>(store, authorization);
    auto metricsRepository = std::make_shared<MetricsRepository>(store, authorization);

    ObservableRepositoryRef observableRepository = userRepository;

    commandHandler_ = std::make_unique<CommandHandler>(observableRepository,
                                                       userRepository,
                                                       discussionThreadRepository,
                                                       discussionThreadMessageRepository,
                                                       discussionTagRepository,
                                                       discussionCategoryRepository,
                                                       authorizationRepository,
                                                       statisticsRepository,
                                                       metricsRepository);

    directWriteRepositories_.user = userRepository;
    directWriteRepositories_.discussionThread = discussionThreadRepository;
    directWriteRepositories_.discussionThreadMessage = discussionThreadMessageRepository;
    directWriteRepositories_.discussionTag = discussionTagRepository;
    directWriteRepositories_.discussionCategory = discussionCategoryRepository;
    directWriteRepositories_.authorization = authorizationRepository;

    const auto forumConfig = Configuration::getGlobalConfig();
    auto& persistenceConfig = forumConfig->persistence;

    try
    {
        persistenceObserver_ = std::make_unique<EventObserver>(observableRepository->readEvents(),
                                                               observableRepository->writeEvents(),
                                                               persistenceConfig.outputFolder,
                                                               persistenceConfig.createNewOutputFileEverySeconds);
        (void)persistenceObserver_; //prevent unused member warnings, no need to use is explicitly
    }
    catch(std::exception& ex)
    {
        FORUM_LOG_FATAL << "Cannot create persistence observer: " << ex.what();
        std::exit(1);
    }
}

void Application::importEvents()
{
    const auto forumConfig = Configuration::getGlobalConfig();
    auto& persistenceConfig = forumConfig->persistence;

    entityCollection_->startBatchInsert();

    EventImporter importer(persistenceConfig.validateChecksum, *entityCollection_, directWriteRepositories_);
    const auto result = importer.import(persistenceConfig.inputFolder);

    entityCollection_->stopBatchInsert();

    if (result.success)
    {
        FORUM_LOG_INFO << "Finished importing " << result.statistic.importedBlobs << " events out of "
                                                << result.statistic.readBlobs << " blobs read";
    }
    else
    {
        FORUM_LOG_ERROR << "Import failed!";
        std::exit(2);
    }
}

void Application::initializeHttp()
{
    const auto forumConfig = Configuration::getGlobalConfig();

    HttpListener::Configuration httpConfig;
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
    const auto forumConfig = Configuration::getGlobalConfig();
    auto& settingsFile = forumConfig->logging.settingsFile;

    if (settingsFile.size() < 1) return;

    std::ifstream file(settingsFile, std::ios::in);
    if ( ! file)
    {
        std::cerr << "Unable to find log settings file: " << settingsFile << '\n';
    }
    try
    {
        boost::log::add_common_attributes();
        boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
        boost::log::register_simple_filter_factory<boost::log::trivial::severity_level>("Severity");
        boost::log::init_from_stream(file);
    }
    catch(std::exception& ex)
    {
        std::cerr << "Unable to load log settings from file: " << ex.what() << '\n';
    }
}
