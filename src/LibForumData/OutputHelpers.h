#pragma once

#include <iosfwd>

#include "JsonWriter.h"

namespace Forum
{
    namespace Helpers
    {
        template <typename T>
        inline void writeSingleValue(std::ostream& output, const char* name, const T& value)
        {
            Json::JsonWriter writer(output);
            writer
                << Json::objStart
                    << Json::property(name, value)
                << Json::objEnd;
        }

        template <typename T>
        inline void writeSingleValueSafeName(std::ostream& output, const char* name, const T& value)
        {
            Json::JsonWriter writer(output);
            writer
                << Json::objStart
                    << Json::propertySafeName(name, value)
                << Json::objEnd;
        }

        inline void writeStatusCode(std::ostream& output, Forum::Repository::StatusCode code)
        {
            writeSingleValueSafeName(output, "status", int(code));
        }

        template <typename TValue>
        inline void writeSingleObjectSafeName(std::ostream& output, const char* name, const TValue& value)
        {
            Json::JsonWriter writer(output);
            writer
                << Json::objStart
                    << Json::propertySafeName(name, value)
                << Json::objEnd;
        }
    }
}
