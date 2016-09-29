#pragma once

#include "Entities.h"
#include "JsonWriter.h"

namespace Json
{
    Json::JsonWriter& operator<<(Json::JsonWriter& writer, const Forum::Entities::User& user);
}
