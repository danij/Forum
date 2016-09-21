#include "MemoryRepository.h"
#include "OutputHelpers.h"

using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

MemoryRepository::MemoryRepository() : collection_(std::make_shared<EntityCollection>())
{
}

void MemoryRepository::getUserCount(std::ostream& output) const
{
    std::size_t count;
    collection_.read([&](const EntityCollection& collection)
                     {
                         count = collection.users_.size();
                     });
    writeSingleValueSafeName(output, "count", count);
}
