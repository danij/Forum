#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include "RandomGenerator.h"

static thread_local boost::uuids::random_generator localUUIDGenerator;

boost::uuids::uuid Forum::Repository::generateUUID()
{
    return localUUIDGenerator();
}

std::string Forum::Repository::generateUUIDString()
{
    return boost::uuids::to_string(generateUUID());
}
