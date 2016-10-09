#include <chrono>

#include "ContextProviders.h"
#include "ContextProviderMocks.h"

Forum::Entities::Timestamp getTimeSinceEpoch()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

static thread_local std::function<Forum::Entities::Timestamp()> getCurrentTimeCallback = getTimeSinceEpoch;

Forum::Entities::Timestamp Forum::Context::getCurrentTime()
{
    return getCurrentTimeCallback();
}

void Forum::Context::setCurrentTimeMockForCurrentThread(std::function<Forum::Entities::Timestamp()> callback)
{
    getCurrentTimeCallback = callback;
}

void Forum::Context::resetCurrentTimeMock()
{
    getCurrentTimeCallback = getTimeSinceEpoch;
}

static thread_local Forum::Entities::IdType currentUser = {};

Forum::Entities::IdType Forum::Context::getCurrentUserId()
{
    return currentUser;
}

void Forum::Context::setCurrentUserId(Forum::Entities::IdType value)
{
    currentUser = value;
}
