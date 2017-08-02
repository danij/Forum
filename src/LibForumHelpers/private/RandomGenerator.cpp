#include "RandomGenerator.h"

#include <boost/uuid/random_generator.hpp>

static thread_local boost::uuids::random_generator localUUIDGenerator;

boost::uuids::uuid Forum::Helpers::generateUUID()
{
    return localUUIDGenerator();
}

Forum::Entities::IdType Forum::Helpers::generateUniqueId()
{
    return Entities::UuidString(generateUUID());
}
