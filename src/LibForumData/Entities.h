#pragma once

//aggregate all entities headers
#include "EntityUser.h"
#include "EntityDiscussionThread.h"
#include "EntityDiscussionMessage.h"
#include "EntityDiscussionTag.h"

namespace Forum
{
    namespace Entities
    {
        struct EntitiesCount
        {
            uint_fast32_t nrOfUsers;
            uint_fast32_t nrOfDiscussionThreads;
            uint_fast32_t nrOfDiscussionMessages;
        };
    }
}