#include "Configuration.h"

#include <boost/property_tree/json_parser.hpp>
#include "ContextProviders.h"

static std::shared_ptr<const Forum::Configuration::Config> currentConfig =
        std::make_shared<const Forum::Configuration::Config>();

std::shared_ptr<const Forum::Configuration::Config> Forum::Configuration::getGlobalConfig()
{
    return std::atomic_load(&currentConfig);
}

void Forum::Configuration::setGlobalConfig(const Config& value)
{
    std::atomic_store(&currentConfig, std::make_shared<const Config>(value));
}

#define CONCAT_MEMBER(structure, member) structure.member
#define LOAD_CONFIG_VALUE(path) \
    CONCAT_MEMBER(config, path) = tree.get<decltype(CONCAT_MEMBER(config, path))>(#path, CONCAT_MEMBER(config, path))

void Forum::Configuration::loadGlobalConfigFromStream(std::ifstream& stream)
{
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(stream, tree);

    Config config;

    LOAD_CONFIG_VALUE(user.minNameLength);
    LOAD_CONFIG_VALUE(user.maxNameLength);
    LOAD_CONFIG_VALUE(user.minInfoLength);
    LOAD_CONFIG_VALUE(user.maxInfoLength);
    LOAD_CONFIG_VALUE(user.minTitleLength);
    LOAD_CONFIG_VALUE(user.maxTitleLength);
    LOAD_CONFIG_VALUE(user.minSignatureLength);
    LOAD_CONFIG_VALUE(user.maxSignatureLength);
    LOAD_CONFIG_VALUE(user.lastSeenUpdatePrecision);
    LOAD_CONFIG_VALUE(user.maxUsersPerPage);

    LOAD_CONFIG_VALUE(discussionThread.minNameLength);
    LOAD_CONFIG_VALUE(discussionThread.maxNameLength);
    LOAD_CONFIG_VALUE(discussionThread.maxUsersInVisitedSinceLastChange);
    LOAD_CONFIG_VALUE(discussionThread.maxThreadsPerPage);

    LOAD_CONFIG_VALUE(discussionThreadMessage.minContentLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxContentLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.minChangeReasonLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxChangeReasonLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxMessagesPerPage);
    LOAD_CONFIG_VALUE(discussionThreadMessage.minCommentLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxCommentLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxMessagesCommentsPerPage);

    LOAD_CONFIG_VALUE(discussionTag.minNameLength);
    LOAD_CONFIG_VALUE(discussionTag.maxNameLength);
    LOAD_CONFIG_VALUE(discussionTag.maxUiBlobSize);

    LOAD_CONFIG_VALUE(discussionCategory.minNameLength);
    LOAD_CONFIG_VALUE(discussionCategory.maxNameLength);
    LOAD_CONFIG_VALUE(discussionCategory.maxDescriptionLength);

    LOAD_CONFIG_VALUE(service.numberOfIOServiceThreads);
    LOAD_CONFIG_VALUE(service.numberOfReadBuffers);
    LOAD_CONFIG_VALUE(service.numberOfWriteBuffers);
    LOAD_CONFIG_VALUE(service.listenIPAddress);
    LOAD_CONFIG_VALUE(service.listenPort);
    LOAD_CONFIG_VALUE(service.connectionTimeoutSeconds);
    LOAD_CONFIG_VALUE(service.trustIpFromXForwardedFor);
    LOAD_CONFIG_VALUE(service.disableCommands);

    LOAD_CONFIG_VALUE(logging.settingsFile);

    LOAD_CONFIG_VALUE(persistence.inputFolder);
    LOAD_CONFIG_VALUE(persistence.outputFolder);
    LOAD_CONFIG_VALUE(persistence.validateChecksum);
    LOAD_CONFIG_VALUE(persistence.createNewOutputFileEverySeconds);

    setGlobalConfig(config);

    Context::setDisableCommands(config.service.disableCommands);
}
