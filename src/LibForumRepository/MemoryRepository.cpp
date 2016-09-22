#include "MemoryRepository.h"
#include "OutputHelpers.h"
#include "StringHelpers.h"

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
                         count = collection.usersById().size();
                     });
    writeSingleValueSafeName(output, "count", count);
}

StatusCode MemoryRepository::addNewUser(const std::string& name, std::ostream& output)
{
    if (stringNullOrEmpty(name))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    return StatusCode::OK;
}
