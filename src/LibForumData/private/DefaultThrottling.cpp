#include "DefaultThrottling.h"

#include <mutex>

using namespace Forum::Entities;
using namespace Forum::Helpers;

bool Forum::Authorization::DefaultThrottling::check(UserActionThrottling action, Timestamp at, const IdType& id,
                                                    const IpAddress& ip)
{
    assert(action < UserActionThrottling::COUNT);

    decltype(entries_)::iterator it;
    {
        std::lock_guard<decltype(lock_)> lock(lock_);
        IdOrIpAddress current(id, ip);

        it = entries_.find(current);
        if (it == entries_.end())
        {
            entries_.insert({ current, UserThrottlingChecks{} });
            it = entries_.find(current);
        }
    }
    return it->second.values[static_cast<EnumIntType>(action)].isAllowed(at);
}
