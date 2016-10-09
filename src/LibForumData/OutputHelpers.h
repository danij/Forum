#pragma once

#include <iosfwd>

#include "JsonWriter.h"
#include "Repository.h"

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

        /**
         * Helper for writing a status message in the output if no other output is provided
         */
        struct StatusWriter : boost::noncopyable
        {
            /**
             * Initializes the helper with the stream to write to and the default status code
             */
            inline StatusWriter(std::ostream& output, Forum::Repository::StatusCode defaultCode) :
                    output_(output), statusCode_(defaultCode), enabled_(true)
            {}

            inline ~StatusWriter()
            {
                if (enabled_) writeStatusCode(output_, statusCode_);
            }

            /**
             * Disables this helper, preventing it to write to the stream.
             * Use this method when something else is to be written to the output
             */
            inline void disable()
            {
                enabled_ = false;
            }

            StatusWriter& operator=(Forum::Repository::StatusCode newCode)
            {
                statusCode_ = newCode;
                return *this;
            }

        private:
            std::ostream& output_;
            Forum::Repository::StatusCode statusCode_;
            bool enabled_;
        };
    }
}