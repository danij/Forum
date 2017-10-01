#pragma once

#include "EntityCommonTypes.h"
#include "AuthorizationPrivileges.h"
#include "ThrottlingCheck.h"
#include "IdOrIpAddress.h"

#include <algorithm>
#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Authorization
    {
        class DefaultThrottling final : private boost::noncopyable
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
                        auto index = i++;
                        return CheckType(ThrottlingDefaultValues[index].first, ThrottlingDefaultValues[index].second);
                    });
                }
                CheckType values[static_cast<EnumIntType>(UserActionThrottling::COUNT)];
            };

            std::unordered_map<Entities::IdOrIpAddress, UserThrottlingChecks> entries_;
            Helpers::SpinLock lock_;
        };
    }
}
