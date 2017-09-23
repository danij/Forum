#include "ContextProviders.h"
#include "ContextProviderMocks.h"

#include <chrono>

using namespace Forum::Context;
using namespace Forum::Entities;
using namespace Forum::Helpers;

Timestamp getTimeSinceEpoch()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

static thread_local std::function<Timestamp()> getCurrentTimeCallback = getTimeSinceEpoch;

Timestamp Forum::Context::getCurrentTime()
{
    return getCurrentTimeCallback();
}

void Forum::Context::setCurrentTimeMockForCurrentThread(std::function<Timestamp()>&& callback)
{
    getCurrentTimeCallback = callback;
}

void Forum::Context::resetCurrentTimeMock()
{
    getCurrentTimeCallback = getTimeSinceEpoch;
}

static thread_local IdType currentUser{};

IdTypeRef Forum::Context::getCurrentUserId()
{
    return currentUser;
}

void Forum::Context::setCurrentUserId(IdType value)
{
    currentUser = std::move(value);
}

static thread_local IpAddress currentIpAddress = {};

const IpAddress& Forum::Context::getCurrentUserIpAddress()
{
    return currentIpAddress;
}

void Forum::Context::setCurrentUserIpAddress(IpAddress value)
{
    currentIpAddress = std::move(value);
}

static bool batchInsertInProgress{ false };

bool Forum::Context::isBatchInsertInProgress()
{
    return batchInsertInProgress;
}

void Forum::Context::setBatchInsertInProgres(bool value)
{
    batchInsertInProgress = value;
}

static thread_local DisplayContext displayContext = {};

const DisplayContext& Forum::Context::getDisplayContext()
{
    return displayContext;
}

DisplayContext& Forum::Context::getMutableDisplayContext()
{
    return displayContext;
}

static std::unique_ptr<IIOServiceProvider> ioServiceProvider;

IIOServiceProvider& Forum::Context::getIOServiceProvider()
{
    return *ioServiceProvider;
}

void Forum::Context::setIOServiceProvider(std::unique_ptr<IIOServiceProvider>&& provider)
{
    if (ioServiceProvider)
    {
        throw std::runtime_error("The IOSErviceProvider should only be set once");
    }
    ioServiceProvider = std::move(provider);
}

static std::unique_ptr<ApplicationEventCollection> applicationEvents;

ApplicationEventCollection& Forum::Context::getApplicationEvents()
{
    return *applicationEvents;
}

void Forum::Context::setApplicationEventCollection(std::unique_ptr<ApplicationEventCollection>&& collection)
{
    if (applicationEvents)
    {
        throw std::runtime_error("The ApplicationEventCollection should only be set once");
    }
    applicationEvents = std::move(collection);
}

static std::atomic_bool disableCommandsAllThreads;

bool Forum::Context::disableCommands()
{
    return disableCommandsAllThreads;
}

void Forum::Context::setDisableCommands(bool value)
{
    disableCommandsAllThreads = value;
}
