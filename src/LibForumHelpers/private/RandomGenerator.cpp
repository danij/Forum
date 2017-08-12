#include "RandomGenerator.h"

#include <mutex>

#include <boost/uuid/random_generator.hpp>

static boost::uuids::random_generator randomUUIDGenerator;
static std::mutex randomUUIDGeneratorMutex;

boost::uuids::uuid Forum::Helpers::generateUUID()
{
    std::lock_guard<std::mutex> guard(randomUUIDGeneratorMutex);
    return randomUUIDGenerator();
}

Forum::Entities::UuidString Forum::Helpers::generateUniqueId()
{
    return Entities::UuidString(generateUUID());
}
