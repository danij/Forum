/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "JsonWriter.h"
#include "Repository.h"
#include "Authorization.h"
#include "AuthorizationGrantedPrivilegeStore.h"

#include <cassert>
#include <tuple>

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

        template<typename Collection, size_t PropertyNameSize, typename FilterType>
        void writeEntitiesWithPagination(const Collection& collection, int_fast32_t pageNumber, int_fast32_t pageSize,
                                         bool ascending, const char(&propertyName)[PropertyNameSize], Json::JsonWriter& writer,
                                         FilterType&& filter, const Authorization::SerializationRestriction& restriction)
        {
            auto totalCount = static_cast<int_fast32_t>(collection.size());

            writer << Json::propertySafeName("totalCount", totalCount)
                   << Json::propertySafeName("pageSize", pageSize)
                   << Json::propertySafeName("page", pageNumber);

            writer.newPropertyWithSafeName(propertyName, PropertyNameSize - 1);
            writer.startArray();

            auto firstElementIndex = std::max(static_cast<decltype(totalCount)>(0),
                                              static_cast<decltype(totalCount)>(pageNumber * pageSize));
            if (ascending)
            {
                for (auto it = collection.nth(firstElementIndex), n = collection.nth(firstElementIndex + pageSize); it != n; ++it)
                {
                    if (*it && filter(**it))
                    {
                        serialize(writer, **it, restriction);
                    }
                }
            }
            else
            {
                auto itStart = collection.nth(std::max(totalCount - firstElementIndex,
                                                       static_cast<decltype(totalCount)>(0)));
                auto itEnd = collection.nth(std::max(totalCount - firstElementIndex - pageSize,
                                                     static_cast<decltype(totalCount)>(0)));

                if (itStart != collection.begin())
                {
                    for (auto it = itStart; it != itEnd;)
                    {
                        --it;
                        if (*it && filter(**it))
                        {
                            serialize(writer, **it, restriction);
                        }
                    }
                }
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
            writeEntitiesWithPagination(collection, pageNumber, pageSize, ascending, propertyName, writer,
                                        std::move(filter), restriction);
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

        template<typename Collection, size_t PropertyNameSize, typename FilterFn>
        void writeAllEntities(const Collection& collection, const char(&propertyName)[PropertyNameSize],
                              Repository::OutStream& output, bool ascending, FilterFn&& filter,
                              const Authorization::SerializationRestriction& restriction)
        {
            Json::JsonWriter writer(output);

            writer.startObject();

            writer.newPropertyWithSafeName(propertyName, PropertyNameSize - 1);
            writer.startArray();

            if (ascending)
            {
                for (auto it = collection.begin(), n = collection.end(); it != n; ++it)
                {
                    if (*it && filter(**it))
                    {
                        serialize(writer, **it, restriction);
                    }
                }
            }
            else
            {
                for (auto it = collection.rbegin(), n = collection.rend(); it != n; ++it)
                {
                    if (*it && filter(**it))
                    {
                        serialize(writer, **it, restriction);
                    }
                }
            }
            writer.endArray();

            writer.endObject();
        }

        template<typename Collection, size_t PropertyNameSize>
        void writeAllEntities(const Collection& collection, const char(&propertyName)[PropertyNameSize],
                              Repository::OutStream& output, bool ascending,
                              const Authorization::SerializationRestriction& restriction)
        {
            writeAllEntities(collection, propertyName, output, ascending, [](auto&) { return true; }, restriction);
        }

        template<typename It, size_t PropertyNameSize>
        void writeAllEntities(It begin, It end, const char(&propertyName)[PropertyNameSize],
                              Repository::OutStream& output, const Authorization::SerializationRestriction& restriction)
        {
            Json::JsonWriter writer(output);

            writer.startObject();

            writer.newPropertyWithSafeName(propertyName, PropertyNameSize - 1);
            writer.startArray();

            for (auto it = begin, n = end; it != n; ++it)
            {
                if (*it)
                {
                    serialize(writer, **it, restriction);
                }
                else
                {
                    writer.null();
                }
            }

            writer.endArray();

            writer.endObject();
        }

        /**
         * Helper for writing a status message in the output if no other output is provided
         */
        struct StatusWriter final : boost::noncopyable
        {
            /**
             * Initializes the helper with the stream to write to and the default status code
             */
            StatusWriter(Repository::OutStream& output) :
                    output_(output), statusCode_(Repository::StatusCode::UNAUTHORIZED), enabled_(true)
            {}

            StatusWriter(const StatusWriter&) = delete;
            StatusWriter(StatusWriter&&) = delete;

            StatusWriter& operator=(const StatusWriter&) = delete;
            StatusWriter& operator=(StatusWriter&&) = delete;

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
