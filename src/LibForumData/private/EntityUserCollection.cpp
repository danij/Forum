#include "EntityUserCollection.h"

using namespace Forum::Entities;

bool UserCollection::add(UserPtr user)
{
    if ( ! std::get<1>(byId_.insert(user))) return false;
    byAuth_.insert(user);
    byName_.insert(user);
    byCreated_.insert(user);
    byLastSeen_.insert(user);
    byThreadCount_.insert(user);
    byMessageCount_.insert(user);

    if (onCountChange_) onCountChange_();
    return true;
}

bool UserCollection::remove(UserPtr user)
{
    {
        auto itById = byId_.find(user);
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    {
        auto itByAuth = byAuth_.find(user);
        if (itByAuth != byAuth_.end()) byAuth_.erase(itByAuth);
    }
    {
        auto itByName = byName_.find(user);
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    byCreated_.erase(user);
    byLastSeen_.erase(user);
    byThreadCount_.erase(user);
    byMessageCount_.erase(user);

    if (onCountChange_) onCountChange_();
    return true;
}

void UserCollection::updateAuth(UserPtr user)
{
    auto it = byAuth_.find(user);
    if (it != byAuth_.end())
    {
        byAuth_.replace(it, user);
    }
}

void UserCollection::updateName(UserPtr user)
{
    auto it = byName_.find(user);
    if (it != byName_.end())
    {
        byName_.replace(it, user);
    }
}

void UserCollection::updateLastSeen(UserPtr user)
{
    auto it = byLastSeen_.find(user);
    if (it != byLastSeen_.end())
    {
        byLastSeen_.replace(it, user);
    }
}

void UserCollection::updateThreadCount(UserPtr user)
{
    auto it = byThreadCount_.find(user);
    if (it != byThreadCount_.end())
    {
        byThreadCount_.replace(it, user);
    }
}

void UserCollection::updateMessageCount(UserPtr user)
{
    auto it = byMessageCount_.find(user);
    if (it != byMessageCount_.end())
    {
        byMessageCount_.replace(it, user);
    }
}
