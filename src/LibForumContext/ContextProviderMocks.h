#pragma once

#include <functional>

#include "ContextProviders.h"

/**
 * This header is only to be included in tests
 */

namespace Forum
{
    namespace Context
    {
        void setCurrentTimeMockForCurrentThread(std::function<Forum::Entities::Timestamp()> callback);
        void resetCurrentTimeMock();
    }
}