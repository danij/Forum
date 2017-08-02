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
#include "MemoryRepositoryStatistics.h"
#include "MetricsRepository.h"

#include "Logging.h"
#include "EventImporter.h"
#include "Version.h"

#include <unicode/uclean.h>

#include <fstream>
#include <iostream>

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

bool Application::initialize(const std::string& configurationFileName)
{
    try
    {
        std::ifstream input(configurationFileName);
        Configuration::loadGlobalConfigFromStream(input);
    }
    catch(std::exception& ex)
    {
        std::cerr << "Error loading configuration: " << ex.what() << '\n';
        return false;
    }

    setApplicationEventCollection(std::make_unique<ApplicationEventCollection>());
    setIOServiceProvider(std::make_unique<DefaultIOServiceProvider>());

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
            ("config", boost::program_options::value<std::string>(), "Specify the location of the configuration file");

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

    auto configFileName = arguments["config"].as<std::string>();

    if ( ! boost::filesystem::exists(configFileName))
    {
        std::cerr << "The configuration file '" << configFileName << "' does not exist!\n";
        return 1;
    }

    if ( ! boost::filesystem::is_regular_file(configFileName))
    {
        std::cerr << "The configuration file '" << configFileName << "' is not a regular file!\n";
        return 1;
    }

    if ( ! initialize(configFileName))
    {
        std::cerr << "Initialization failed!\n";
        return 1;
    }

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
    entityCollection_ = std::make_shared<Entities::EntityCollection>();

    auto store = std::make_shared<MemoryStore>(entityCollection_);
    auto authorization = std::make_shared<DefaultAuthorization>(entityCollection_->grantedPrivileges(), *entityCollection_);

    auto userRepository = std::make_shared<MemoryRepositoryUser>(store, authorization);
    auto discussionThreadRepository = std::make_shared<MemoryRepositoryDiscussionThread>(store, authorization);
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
                                                       statisticsRepository,
                                                       metricsRepository);

    directWriteRepositories_.user = userRepository;
    directWriteRepositories_.discussionThread = discussionThreadRepository;
    directWriteRepositories_.discussionThreadMessage = discussionThreadMessageRepository;
    directWriteRepositories_.discussionTag = discussionTagRepository;
    directWriteRepositories_.discussionCategory = discussionCategoryRepository;

    auto forumConfig = Configuration::getGlobalConfig();
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
    auto forumConfig = Configuration::getGlobalConfig();
    auto& persistenceConfig = forumConfig->persistence;

    EventImporter importer(persistenceConfig.validateChecksum, *entityCollection_, directWriteRepositories_);
    auto result = importer.import(persistenceConfig.inputFolder);
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
    auto forumConfig = Configuration::getGlobalConfig();

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
