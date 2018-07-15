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

#include "EntityCommonTypes.h"
#include "IpAddress.h"

#include <tuple>
#include <variant>

namespace Forum::Entities
{
    /**
     * Stores both an id and an IP address
     */
    struct IdOrIpAddress final
    {
        IdOrIpAddress(const IdType& id, const Helpers::IpAddress& ip)
        {
            if (id)
            {
                data_ = id;
            }
            else
            {
                data_ = ip;
            }
        }

        IdOrIpAddress(const IdOrIpAddress& other) = default;
        IdOrIpAddress(IdOrIpAddress&&) = default;

        IdOrIpAddress& operator=(const IdOrIpAddress&) = default;
        IdOrIpAddress& operator=(IdOrIpAddress&&) = default;

        bool operator==(const IdOrIpAddress& other) const
        {
            return getCompareStructure() == other.getCompareStructure();
        }

        bool operator!=(const IdOrIpAddress& other) const
        {
            return getCompareStructure() != other.getCompareStructure();
        }

        bool operator<(const IdOrIpAddress& other) const
        {
            return getCompareStructure() < other.getCompareStructure();
        }

        bool operator<=(const IdOrIpAddress& other) const
        {
            return getCompareStructure() <= other.getCompareStructure();
        }
        bool operator>(const IdOrIpAddress& other) const
        {
            return getCompareStructure() > other.getCompareStructure();
        }

        bool operator>=(const IdOrIpAddress& other) const
        {
            return getCompareStructure() >= other.getCompareStructure();
        }

    private:
        std::tuple<IdType, Helpers::IpAddress> getCompareStructure() const
        {
            const auto* idPtr = std::get_if<IdType>(&data_);
            const auto* ipPtr = std::get_if<Helpers::IpAddress>(&data_);

            return{ idPtr ? *idPtr : IdType{}, ipPtr ? *ipPtr : Helpers::IpAddress{} };
        }

        friend struct std::hash<IdOrIpAddress>;

        std::variant<IdType, Helpers::IpAddress> data_;
    };
}

namespace std
{
    template<>
    struct hash<Forum::Entities::IdOrIpAddress>
    {
        size_t operator()(const Forum::Entities::IdOrIpAddress& value) const
        {
            if (const auto* idPtr = std::get_if<Forum::Entities::IdType>(&value.data_))
            {
                return hash<Forum::Entities::IdType>()(*idPtr);
            }

            if (const auto* ipPtr = std::get_if<Forum::Helpers::IpAddress>(&value.data_))
            {
                return hash<Forum::Helpers::IpAddress>()(*ipPtr);
            }
            return{};
        }
    };
}
