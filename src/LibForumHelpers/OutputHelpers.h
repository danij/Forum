#pragma once

#include "JsonWriter.h"
#include "Repository.h"

#include <iosfwd>
#include <array>

namespace Forum
{
    namespace Helpers
    {
        template <typename T, std::size_t Size>
        void writeSingleValueSafeName(Forum::Repository::OutStream& output, const char(&name)[Size], const T& value)
        {
            Json::JsonWriter writer(output);
            writer
                << Json::objStart
                    << Json::propertySafeName<Size, const T&>(name, value)
                << Json::objEnd;
        }

        inline void writeStatusCode(Forum::Repository::OutStream& output, Repository::StatusCode code)
        {
            writeSingleValueSafeName(output, "status", code);
        }

        template<typename Collection, typename InterceptorFn, std::size_t PropertyNameSize>
        void writeEntitiesWithPagination(const Collection& collection, const char(&propertyName)[PropertyNameSize],
                                         Json::JsonWriter& writer, int_fast32_t pageNumber, int_fast32_t pageSize,
                                         bool ascending, InterceptorFn&& interceptor)
        {
            auto count = static_cast<int_fast32_t>(collection.size());

            typedef decltype(collection.begin()) IteratorType;

            IteratorType itStart, itEnd;

            auto firstElementIndex = std::max(static_cast<decltype(count)>(0), 
                                              static_cast<decltype(count)>(pageNumber * pageSize));
            if (ascending)
            {

                itStart = collection.nth(firstElementIndex);
                itEnd = collection.nth(firstElementIndex + pageSize);
            }
            else
            {
                itStart = collection.nth(std::max(count - firstElementIndex, static_cast<decltype(count)>(0)));
                itEnd = collection.nth(std::max(count - firstElementIndex - pageSize, static_cast<decltype(count)>(0)));
            }

            writer << Json::propertySafeName("totalCount", count)
                   << Json::propertySafeName("pageSize", pageSize)
                   << Json::propertySafeName("page", pageNumber);

            //need to write the array manually, so that we can control the advance direction of iteration
            writer.newPropertyWithSafeName(propertyName, PropertyNameSize - 1);
            writer << Json::arrayStart;
            if (ascending)
            {
                while (itStart != itEnd)
                {
                    writer << interceptor(*itStart);
                    ++itStart;
                }
            }
            else
            {
                auto start = collection.begin();
                if (itStart != start)
                {
                    while (itStart != itEnd)
                    {
                        writer << interceptor(*--itStart);
                    }
                }
            }
            writer << Json::arrayEnd;
        }
        
        template<typename Collection, typename InterceptorFn, std::size_t PropertyNameSize>
        void writeEntitiesWithPagination(const Collection& collection, const char(&propertyName)[PropertyNameSize],
                                         Forum::Repository::OutStream& output, int_fast32_t pageNumber,
                                         int_fast32_t pageSize, bool ascending, InterceptorFn&& interceptor)
        {
            Json::JsonWriter writer(output);
            writer << Json::objStart;

            writeEntitiesWithPagination(collection, propertyName, writer, pageNumber, pageSize, ascending, interceptor);

            writer << Json::objEnd;
        }

        /**
         * Helper for writing a status message in the output if no other output is provided
         */
        struct StatusWriter : boost::noncopyable
        {
            /**
             * Initializes the helper with the stream to write to and the default status code
             */
            StatusWriter(Forum::Repository::OutStream& output, Repository::StatusCode defaultCode) :
                    output_(output), statusCode_(defaultCode), enabled_(true)
            {}

            ~StatusWriter()
            {
                if ( ! enabled_) return;

                Json::JsonWriter writer(output_);

                writer << Json::objStart;
                writer << Json::propertySafeName("status", statusCode_);
                for (int i = 0; i < elementCount_; ++i)
                {
                    extras_[i](writer);
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

            operator Repository::StatusCode() const
            {
                return statusCode_;
            }

            /**
             * Adds extra information to be written
             */
            template <typename T>
            void addExtraSafeName(const std::string& key, const T& value)
            {
                extras_.at(elementCount_++) = [=](auto& writer)
                                              {
                                                  writer << Json::propertySafeName(key, value);
                                              };
            }

        private:
            Forum::Repository::OutStream& output_;
            Repository::StatusCode statusCode_;
            bool enabled_;
            int elementCount_ = 0;
            //maximum count should be adjusted if needed, the value is known at compile time
            std::array<std::function<void(Json::JsonWriter&)>, 10> extras_;
        };
    }
}
