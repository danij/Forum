#include "ContextProviders.h"
#include "ContextProviderMocks.h"

#include <chrono>

using namespace Forum::Context;
using namespace Forum::Entities;

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

void Forum::Context::setCurrentTimeMockForCurrentThread(std::function<Timestamp()> callback)
{
    getCurrentTimeCallback = callback;
}

void Forum::Context::resetCurrentTimeMock()
{
    getCurrentTimeCallback = getTimeSinceEpoch;
}

static thread_local IdType currentUser{};

IdType Forum::Context::getCurrentUserId()
{
    return currentUser;
}

void Forum::Context::setCurrentUserId(IdType value)
{
    currentUser = value;
}

static thread_local std::string currentIpAddress{};

std::string Forum::Context::getCurrentUserIpAddress()
{
    return currentIpAddress;
}

void Forum::Context::setCurrentUserIpAddress(const std::string& value)
{
    currentIpAddress = value;
}

static thread_local std::string currentUserAgent{};

std::string Forum::Context::getCurrentUserBrowserUserAgent()
{
    return currentUserAgent;
}

void Forum::Context::setCurrentUserBrowserUserAgent(const std::string& value)
{
    currentUserAgent = value;
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
