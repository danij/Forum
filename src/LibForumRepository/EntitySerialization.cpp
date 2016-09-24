#include <boost/uuid/uuid_io.hpp>

#include "EntitySerialization.h"

using namespace Forum::Entities;
using namespace Json;

JsonWriter& Json::operator<<(JsonWriter& writer, const Forum::Entities::User& user)
{
    writer
        << objStart
            << propertySafeName("id", boost::uuids::to_string(user.id()))
            << propertySafeName("name", user.name())
        << objEnd;
    return writer;
}
