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
        auto itById = byId_.find(user->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    {
        auto itByAuth = byAuth_.find(user->auth());
        if (itByAuth != byAuth_.end()) byAuth_.erase(itByAuth);
    }
    {
        auto itByName = byName_.find(user->name());
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    eraseFromNonUniqueCollection(byCreated_, user, user->created());
    eraseFromNonUniqueCollection(byLastSeen_, user, user->lastSeen());
    eraseFromNonUniqueCollection(byThreadCount_, user, user->threadCount());
    eraseFromNonUniqueCollection(byMessageCount_, user, user->messageCount());

    if (onCountChange_) onCountChange_();
    return true;
}

void UserCollection::updateAuth(UserPtr user)
{
    auto it = byAuth_.find(user->auth());
    if (it != byAuth_.end())
    {
        byAuth_.replace(it, user);
    }
}

void UserCollection::updateName(UserPtr user)
{
    auto it = byName_.find(user->name());
    if (it != byName_.end())
    {
        byName_.replace(it, user);
    }
}

void UserCollection::updateLastSeen(UserPtr user)
{
    replaceInNonUniqueCollection(byLastSeen_, user, user->lastSeen());
}

void UserCollection::updateThreadCount(UserPtr user)
{
    replaceInNonUniqueCollection(byThreadCount_, user, user->threadCount());
}

void UserCollection::updateMessageCount(UserPtr user)
{
    replaceInNonUniqueCollection(byMessageCount_, user, user->messageCount());
}
