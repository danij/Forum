#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include "RandomGenerator.h"

static thread_local boost::uuids::random_generator localUUIDGenerator;

boost::uuids::uuid Forum::Repository::generateUUID()
{
    return localUUIDGenerator();
}

Forum::Entities::UuidString Forum::Repository::generateUUIDString()
{
    return Entities::UuidString(generateUUID());
}
