#include <boost/regex/icu.hpp>

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "MemoryRepository.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

void MemoryRepository::getDiscussionThreadCount(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         auto count = collection.threadsById().size();
                         writeSingleValueSafeName(output, "count", count);

                         observers_.getDiscussionThreadCount(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByName(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByName();
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.getDiscussionThreads(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByCreated(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByCreated();
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.getDiscussionThreads(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByLastUpdated(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByLastUpdated();
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.getDiscussionThreads(performedBy.get(collection));
                     });
}
