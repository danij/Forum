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
#include "AuthorizationPrivileges.h"
#include "ThrottlingCheck.h"
#include "IdOrIpAddress.h"

#include <algorithm>
#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace Forum::Authorization
{
    class DefaultThrottling final : boost::noncopyable
    {
    public:
        bool check(UserActionThrottling action, Entities::Timestamp at, const Entities::IdType& id,
                   const Helpers::IpAddress& ip);

    private:
        struct UserThrottlingChecks
        {
            typedef ThrottlingCheck<Entities::Timestamp> CheckType;

            UserThrottlingChecks()
            {
                EnumIntType i = 0;
                std::generate(std::begin(values), std::end(values), [&i]()
                {
                    const auto index = i++;
                    return CheckType(ThrottlingDefaultValues[index].first, ThrottlingDefaultValues[index].second);
                });
            }
            CheckType values[static_cast<EnumIntType>(UserActionThrottling::COUNT)];
        };

        std::unordered_map<Entities::IdOrIpAddress, UserThrottlingChecks> entries_;
        Helpers::SpinLock lock_;
    };
}
