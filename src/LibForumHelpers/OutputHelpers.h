#pragma once

#include "JsonWriter.h"
#include "Repository.h"
#include "Authorization.h"
#include "AuthorizationGrantedPrivilegeStore.h"

#include <cassert>
#include <tuple>

#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/filtered.hpp>

namespace Forum
{
    namespace Helpers
    {
        template <typename T, size_t Size>
        void writeSingleValueSafeName(Repository::OutStream& output, const char(&name)[Size], const T& value)
        {
            Json::JsonWriter writer(output);
            writer
                << Json::objStart
                    << Json::propertySafeName<Size, const T&>(name, value)
                << Json::objEnd;
        }

        template <typename T, size_t Size>
        void writeSingleValueSafeName(Repository::OutStream& output, const char(&name)[Size], const T& value,
                                      const Authorization::SerializationRestriction& restriction)
        {
            Json::JsonWriter writer(output);

            writer.startObject();
            writer.newPropertyWithSafeName(name);
            serialize(writer, value, restriction);
            writer.endObject();
        }

        template <typename It, size_t Size>
        void writeArraySafeName(Json::JsonWriter& writer, const char(&name)[Size], It begin, It end,
                                const Authorization::SerializationRestriction& restriction)
        {
            writer.newPropertyWithSafeName(name);
            writer.startArray();
            for (auto it = begin; it != end; ++it)
            {
                serialize(writer, **it, restriction);
            }
            writer.endArray();
        }

        template <typename It, size_t Size>
        void writeArraySafeName(Repository::OutStream& output, const char(&name)[Size], It begin, It end,
                                const Authorization::SerializationRestriction& restriction)
        {
            Json::JsonWriter writer(output);
            writer.startObject();
            writeArraySafeName(writer, name, begin, end, restriction);
            writer.endObject();
        }

        inline void writeStatusCode(Repository::OutStream& output, Repository::StatusCode code)
        {
            writeSingleValueSafeName(output, "status", code);
        }

        template<typename Collection, typename FilterFn>
        auto getPageFromCollection(const Collection& collection, int_fast32_t pageNumber, int_fast32_t pageSize, 
                                   bool ascending, FilterFn&& filter)
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

                if (itStart == collection.begin())
                {
                    itStart = itEnd;
                }
            }
            return std::make_tuple(count, pageSize, pageNumber, 
                                   boost::make_iterator_range(itStart, itEnd) | boost::adaptors::filtered(filter));
        }

        template<typename PageInfoType, size_t PropertyNameSize>
        void writeEntitiesWithPagination(const PageInfoType& pageInfo, const char(&propertyName)[PropertyNameSize],
                                         Json::JsonWriter& writer, const Authorization::SerializationRestriction& restriction)
        {
            writer << Json::propertySafeName("totalCount", std::get<0>(pageInfo))
                   << Json::propertySafeName("pageSize", std::get<1>(pageInfo))
                   << Json::propertySafeName("page", std::get<2>(pageInfo));

            writer.newPropertyWithSafeName(propertyName, PropertyNameSize - 1);
            writer.startArray();
            for (const auto& value : std::get<3>(pageInfo))
            {
                serialize(writer, *value, restriction);
            }
            writer.endArray();
        }

        template<typename Collection, size_t PropertyNameSize, typename FilterFn>
        void writeEntitiesWithPagination(const Collection& collection, const char(&propertyName)[PropertyNameSize],
                                         Repository::OutStream& output, int_fast32_t pageNumber,
                                         int_fast32_t pageSize, bool ascending, FilterFn&& filter,
                                         const Authorization::SerializationRestriction& restriction)
        {
            Json::JsonWriter writer(output);

            writer.startObject();
            auto pageInfo = getPageFromCollection(collection, pageNumber, pageSize, ascending, std::move(filter));
            writeEntitiesWithPagination(pageInfo, propertyName, writer, restriction);
            writer.endObject();
        }

        template<typename Collection, size_t PropertyNameSize>
        void writeEntitiesWithPagination(const Collection& collection, const char(&propertyName)[PropertyNameSize],
                                         Repository::OutStream& output, int_fast32_t pageNumber,
                                         int_fast32_t pageSize, bool ascending,
                                         const Authorization::SerializationRestriction& restriction)
        {
            writeEntitiesWithPagination(collection, propertyName, output, pageNumber, pageSize, ascending, 
                                        [](auto&) { return true; }, restriction);
        }

        /**
         * Helper for writing a status message in the output if no other output is provided
         */
        struct StatusWriter : boost::noncopyable
        {
            /**
             * Initializes the helper with the stream to write to and the default status code
             */
            StatusWriter(Repository::OutStream& output) :
                    output_(output), statusCode_(Repository::StatusCode::UNAUTHORIZED), enabled_(true)
            {}

            ~StatusWriter()
            {
                if ( ! enabled_) return;

                Json::JsonWriter writer(output_);

                writer << Json::objStart
                       << Json::propertySafeName("status", statusCode_)
                       << Json::objEnd;
            }

            /**
             * Disables this helper, preventing it to write to the stream.
             * Use this method when something else is to be written to the output
             */
            void disable()
            {
                assert(*this);
                enabled_ = false;
            }

            StatusWriter& operator=(Repository::StatusCode newCode)
            {
                statusCode_ = newCode;
                return *this;
            }

            StatusWriter& operator=(Authorization::AuthorizationStatus newCode)
            {
                switch (newCode)
                {
                case Authorization::AuthorizationStatus::NOT_ALLOWED:
                    statusCode_ = Repository::StatusCode::UNAUTHORIZED;
                    break;
                case Authorization::AuthorizationStatus::THROTTLED:
                    statusCode_ = Repository::StatusCode::THROTTLED;
                    break;
                default:
                    statusCode_ = Repository::StatusCode::OK;
                }
                return *this;
            }

            operator bool() const 
            {
                return Repository::StatusCode::OK == statusCode_;
            }

            operator Repository::StatusCode() const
            {
                return statusCode_;
            }

            template<typename Action>
            void writeNow(Action&& extra)
            {
                Json::JsonWriter writer(output_);

                writer << Json::objStart;
                writer << Json::propertySafeName("status", statusCode_);

                extra(writer);

                writer << Json::objEnd;

                //prevent writing again on destructor call
                enabled_ = false;
            }

        private:
            Repository::OutStream& output_;
            Repository::StatusCode statusCode_;
            bool enabled_;
        };
    }
}
