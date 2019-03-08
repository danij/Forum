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

#include <cstring>
#include <memory>
#include <string_view>

/**
 * To be used when declaring an interface.
 * Move constructor/assignment operator is not implicitly declared if a destructor has been declared.
 * Declaring the move constructor then also requires the declaration of the default and copy constructors
 */
#define DECLARE_INTERFACE_MANDATORY(TypeName) \
    TypeName() = default; \
    virtual ~TypeName() = default; \
    TypeName(const TypeName&) = default; \
    TypeName(TypeName&&) = default; \
    TypeName& operator=(const TypeName&) = default; \
    TypeName& operator=(TypeName&&) = default;

#define DECLARE_INTERFACE_MANDATORY_NO_COPY(TypeName) \
    TypeName() = default; \
    virtual ~TypeName() = default; \
    TypeName(const TypeName&) = delete; \
    TypeName(TypeName&&) = default; \
    TypeName& operator=(const TypeName&) = delete; \
    TypeName& operator=(TypeName&&) = default;

#define DECLARE_ABSTRACT_MANDATORY(TypeName) DECLARE_INTERFACE_MANDATORY(TypeName)

#define DECLARE_ABSTRACT_MANDATORY_NO_COPY(TypeName) DECLARE_INTERFACE_MANDATORY_NO_COPY(TypeName)

typedef std::string_view StringView;

namespace Forum::Helpers
{
    template<typename T>
    auto toConstPtr(T* value)
    {
        return static_cast<const T*>(value);
    }

    template<typename T>
    bool ownerEqual(const std::weak_ptr<T>& first, const std::weak_ptr<T>& second)
    {
        return ! first.owner_before(second) && ! second.owner_before(first);
    }

    template<typename T>
    void writeValue(char* destination, T value)
    {
        memmove(destination, &value, sizeof(T));
    }

    template<typename T>
    void readValue(const char* source, T& value)
    {
        memmove(&value, source, sizeof(T));
    }

    template<typename T>
    void readValue(const unsigned char* source, T& value)
    {
        memmove(&value, source, sizeof(T));
    }

    template<typename T>
    T absValue(T value)
    {
        return value < static_cast<T>(0) ? -value : value;
    }
}
