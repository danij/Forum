#pragma once

#include "EntityCommonTypes.h"
#include "IpAddress.h"

#include <tuple>

#include <boost/variant.hpp>

namespace Forum
{
    namespace Entities
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
                const auto* idPtr = boost::get<IdType>(&data_);
                const auto* ipPtr = boost::get<Helpers::IpAddress>(&data_);

                return{ idPtr ? *idPtr : IdType{}, ipPtr ? *ipPtr : Helpers::IpAddress{} };
            }

            friend struct std::hash<IdOrIpAddress>;

            boost::variant<IdType, Helpers::IpAddress> data_;
        };
    }
}

namespace std
{
    template<>
    struct hash<Forum::Entities::IdOrIpAddress>
    {
        size_t operator()(const Forum::Entities::IdOrIpAddress& value) const
        {
            const auto* idPtr = boost::get<Forum::Entities::IdType>(&value.data_);
            const auto* ipPtr = boost::get<Forum::Helpers::IpAddress>(&value.data_);

            if (idPtr)
            {
                return hash<Forum::Entities::IdType>()(*idPtr);
            }
            if (ipPtr)
            {
                return hash<Forum::Helpers::IpAddress>()(*ipPtr);
            }
            return{};
        }
    };
}
