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
        void setCurrentTimeMockForCurrentThread(std::function<Entities::Timestamp()> callback);
        void resetCurrentTimeMock();
    }
}