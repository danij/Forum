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
#include "ConnectionManagerWithTimeout.h"
#include "FixedHttpConnectionManager.h"
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
#include "MemoryRepositoryAttachment.h"
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

#include <boost/dll.hpp>
#include <boost/filesystem.hpp>

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
using namespace Forum::Configuration;
using namespace Forum::Context;
using namespace Forum::Extensibility;
using namespace Forum::Network;
using namespace Forum::Persistence;
using namespace Forum::Repository;

using namespace Http;

bool Application::initialize()
{
    setApplicationEventCollection(std::make_unique<ApplicationEventCollection>());
    setIOServiceProvider(std::make_unique<DefaultIOServiceProvider>(
            Configuration::getGlobalConfig()->service.numberOfIOServiceThreads));

    if ( ! initializeLogging()) return false;

    FORUM_LOG_INFO << "Starting Forum Backend v" << VERSION;

    return createCommandHandler() && importEvents() && loadPlugins() && initializeHttp();
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

    {
        FORUM_LOG_INFO << "Starting to listen under "
                       << config->service.listenIPAddress << ":" << config->service.listenPort;
        try
        {
            tcpListener_->startListening();
        }
        catch (std::exception& ex)
        {
            FORUM_LOG_ERROR << "Could not start listening: " << ex.what();
            std::cerr << "Could not start listening: " << ex.what() << '\n';
            return 1;
        }
    }
    {
        FORUM_LOG_INFO << "Starting to listen for auth requests under "
                       << config->service.authListenIPAddress << ":" << config->service.authListenPort;
        try
        {
            tcpListenerAuth_->startListening();
        }
        catch (std::exception& ex)
        {
            FORUM_LOG_ERROR << "Could not start listening: " << ex.what();
            std::cerr << "Could not start listening: " << ex.what() << '\n';
            return 1;
        }
    }

    getIOServiceProvider().start();
    getIOServiceProvider().waitForStop();

    tcpListenerAuth_->stopListening();
    tcpListener_->stopListening();

    FORUM_LOG_INFO << "Stopped listening for HTTP connections";

    prepareToStop();

    cleanup();

    return 0;
}

void Application::cleanup()
{
    Helpers::cleanupStringHelpers();

    //clean up resources cached by ICU so that they don't show up as memory leaks
    u_cleanup();
}

bool Application::loadConfiguration(const std::string& fileName)
{
    try
    {
        if ( ! boost::filesystem::exists(fileName))
        {
            std::cerr << "The configuration file '" << fileName << "' does not exist!\n";
            return false;
        }

        if ( ! boost::filesystem::is_regular_file(fileName))
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

bool Application::createCommandHandler()
{
    const auto config = Configuration::getGlobalConfig();
    entityCollection_ = std::make_shared<Entities::EntityCollection>(config->persistence.messagesFile);

    auto store = memoryStore_ = std::make_shared<MemoryStore>(entityCollection_);    
    auto authorization = std::make_shared<DefaultAuthorization>(entityCollection_->grantedPrivileges(),
                                                                *entityCollection_, config->service.disableThrottling);

    auto authorizationRepository = std::make_shared<MemoryRepositoryAuthorization>(
            store, authorization, authorization, authorization, authorization, authorization);

    auto userRepository = std::make_shared<MemoryRepositoryUser>(store, authorization, authorizationRepository);
    auto discussionThreadRepository = std::make_shared<MemoryRepositoryDiscussionThread>(store, authorization,
                                                                                         authorizationRepository);
    auto discussionThreadMessageRepository = 
            std::make_shared<MemoryRepositoryDiscussionThreadMessage>(store, authorization, authorizationRepository);
    auto discussionTagRepository = std::make_shared<MemoryRepositoryDiscussionTag>(store, authorization);
    auto discussionCategoryRepository = std::make_shared<MemoryRepositoryDiscussionCategory>(store, authorization);
    auto attachmentRepository = std::make_shared<MemoryRepositoryAttachment>(store, authorization);
    auto statisticsRepository = std::make_shared<MemoryRepositoryStatistics>(store, authorization);
    auto metricsRepository = std::make_shared<MetricsRepository>(store, authorization);

    ObservableRepositoryRef observableRepository = userRepository;

    commandHandler_ = std::make_unique<CommandHandler>(observableRepository,
                                                       userRepository,
                                                       discussionThreadRepository,
                                                       discussionThreadMessageRepository,
                                                       discussionTagRepository,
                                                       discussionCategoryRepository,                                    
                                                       attachmentRepository,                                    
                                                       authorizationRepository,
                                                       statisticsRepository,
                                                       metricsRepository);

    directWriteRepositories_.user = userRepository;
    directWriteRepositories_.discussionThread = discussionThreadRepository;
    directWriteRepositories_.discussionThreadMessage = discussionThreadMessageRepository;
    directWriteRepositories_.discussionTag = discussionTagRepository;
    directWriteRepositories_.discussionCategory = discussionCategoryRepository;
    directWriteRepositories_.attachment = attachmentRepository;
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

        FORUM_LOG_INFO << "Initialized command handlers";

        return true;
    }
    catch(std::exception& ex)
    {
        FORUM_LOG_FATAL << "Cannot create persistence observer: " << ex.what();
        return false;
    }
}

bool Application::importEvents()
{
    FORUM_LOG_INFO << "Starting import of persisted events";

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
        return true;
    }
    else
    {
        FORUM_LOG_ERROR << "Import failed!";
        return false;
    }
}

bool Application::initializeHttp()
{
    const auto forumConfig = Configuration::getGlobalConfig();
    
    auto& ioService = getIOServiceProvider().getIOService();

    endpointManager_ = std::make_unique<ServiceEndpointManager>(*commandHandler_);

    {
        //API listener
        auto httpRouter = std::make_unique<HttpRouter>();
        endpointManager_->registerRoutes(*httpRouter);

        auto httpConnectionManager = std::make_shared<FixedHttpConnectionManager>(std::move(httpRouter),
            forumConfig->service.connectionPoolSize,
            forumConfig->service.numberOfReadBuffers,
            forumConfig->service.numberOfWriteBuffers,
            forumConfig->service.trustIpFromXForwardedFor);

        auto connectionManagerWithTimeout = std::make_shared<ConnectionManagerWithTimeout>(ioService,
            httpConnectionManager, forumConfig->service.connectionTimeoutSeconds);

        tcpListener_ = std::make_unique<TcpListener>(ioService,
            forumConfig->service.listenIPAddress,
            forumConfig->service.listenPort,
            connectionManagerWithTimeout);
    }
    {
        //auth API listener
        auto httpRouterAuth = std::make_unique<HttpRouter>();
        endpointManager_->registerAuthRoutes(*httpRouterAuth);

        auto httpConnectionManagerAuth = std::make_shared<FixedHttpConnectionManager>(std::move(httpRouterAuth),
            forumConfig->service.connectionPoolSize,
            forumConfig->service.numberOfReadBuffers,
            forumConfig->service.numberOfWriteBuffers,
            false);

        auto connectionManagerWithTimeoutAuth = std::make_shared<ConnectionManagerWithTimeout>(ioService,
            httpConnectionManagerAuth, forumConfig->service.connectionTimeoutSeconds);

        tcpListenerAuth_ = std::make_unique<TcpListener>(ioService,
            forumConfig->service.authListenIPAddress,
            forumConfig->service.authListenPort,
            connectionManagerWithTimeoutAuth);
    }
    return true;
}

bool Application::initializeLogging()
{
    const auto forumConfig = Configuration::getGlobalConfig();
    auto& settingsFile = forumConfig->logging.settingsFile;

    if (settingsFile.empty()) return true;

    std::ifstream file(settingsFile, std::ios::in);
    if ( ! file)
    {
        std::cerr << "Unable to find log settings file: " << settingsFile << '\n';
        return false;
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
        return false;
    }
    return true;
}

LoadedPlugin loadPlugin(const PluginEntry& entry, MemoryStore& memoryStore)
{
    FORUM_LOG_INFO << "\tLoading plugin from " << entry.libraryPath;

    try
    {
        boost::dll::shared_library library(entry.libraryPath);
        auto loadFn = library.get<PluginLoaderFn>("loadPlugin");

        PluginInput input
        {
            &Entities::Private::getGlobalEntityCollection(),
            &memoryStore.readEvents,
            &memoryStore.writeEvents,
            &entry.configuration
        };
        PluginPtr plugin;

        loadFn(&input, &plugin);

        if (plugin)
        {
            FORUM_LOG_INFO << "\t\tLoaded " << plugin->name() << " (version " << plugin->version() << ")";
        }

        return 
        {
            std::move(library),
            std::move(plugin)
        };
    }
    catch (std::exception& ex)
    {
        FORUM_LOG_ERROR << "Unable to load plugin: " << ex.what();
        return {};
    }
}

bool Application::loadPlugins()
{
    FORUM_LOG_INFO << "Loading plugins";

    const auto forumConfig = Configuration::getGlobalConfig();

    for (const auto& entry : forumConfig->plugins)
    {
        auto result = loadPlugin(entry, *memoryStore_);
        if ( ! result.plugin) return false;

        plugins_.emplace_back(std::move(result));
    }

    return true;
}

void Application::prepareToStop()
{
    for (auto loadedPlugin : plugins_)
    {
        loadedPlugin.plugin->stop();
    }

    getApplicationEvents().beforeApplicationStop();

    persistenceObserver_.reset();
}
