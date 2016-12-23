#pragma once

#include "JsonWriter.h"
#include "Repository.h"

#include <iosfwd>
#include <vector>

namespace Forum
{
    namespace Helpers
    {
        template <typename T>
        void writeSingleValue(std::ostream& output, const char* name, const T& value)
        {
            Json::JsonWriter writer(output);
            writer
                << Json::objStart
                    << Json::property(name, value)
                << Json::objEnd;
        }

        template <typename T>
        void writeSingleValueSafeName(std::ostream& output, const char* name, const T& value)
        {
            Json::JsonWriter writer(output);
            writer
                << Json::objStart
                    << Json::propertySafeName(name, value)
                << Json::objEnd;
        }

        inline void writeStatusCode(std::ostream& output, Repository::StatusCode code)
        {
            writeSingleValueSafeName(output, "status", code);
        }

        template <typename TValue>
        void writeSingleObjectSafeName(std::ostream& output, const char* name, const TValue& value)
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
            StatusWriter(std::ostream& output, Repository::StatusCode defaultCode) :
                    output_(output), statusCode_(defaultCode), enabled_(true)
            {}

            ~StatusWriter()
            {
                if ( ! enabled_) return;

                Json::JsonWriter writer(output_);

                writer << Json::objStart;
                writer << Json::propertySafeName("status", statusCode_);
                for (auto& extra : extras_)
                {
                    extra(writer);
                }
                writer << Json::objEnd;
            }

            /**
             * Disables this helper, preventing it to write to the stream.
             * Use this method when something else is to be written to the output
             */
            void disable()
            {
                enabled_ = false;
            }

            StatusWriter& operator=(Repository::StatusCode newCode)
            {
                statusCode_ = newCode;
                return *this;
            }

            /**
             * Adds extra information to be written
             */
            template <typename T>
            void addExtraSafeName(const std::string& key, const T& value)
            {
                extras_.push_back([=](auto& writer)
                                  {
                                      writer << Json::propertySafeName(key, value);
                                  });
            }

        private:
            std::ostream& output_;
            Repository::StatusCode statusCode_;
            bool enabled_;
            std::vector<std::function<void(Json::JsonWriter&)>> extras_;
        };
    }
}
