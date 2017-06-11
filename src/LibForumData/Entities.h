#pragma once

//aggregate all entities headers
#include "EntityUser.h"
#include "EntityDiscussionThread.h"
#include "EntityDiscussionThreadMessage.h"
#include "EntityMessageComment.h"
#include "EntityDiscussionTag.h"
#include "EntityDiscussionCategory.h"

namespace Forum
{
    namespace Entities
    {
        struct EntitiesCount
        {
            uint_fast32_t nrOfUsers;
            uint_fast32_t nrOfDiscussionThreads;
            uint_fast32_t nrOfDiscussionMessages;
            uint_fast32_t nrOfDiscussionTags;
            uint_fast32_t nrOfDiscussionCategories;
        };
    }
}
