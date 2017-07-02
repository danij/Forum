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

    return true;
}

void UserCollection::prepareUpdateAuth(UserPtr user)
{
    byAuthUpdateIt_ = byAuth_.find(user->auth());
}

void UserCollection::updateAuth(UserPtr user)
{
    if (byAuthUpdateIt_ != byAuth_.end())
    {
        byAuth_.replace(byAuthUpdateIt_, user);
    }
}

void UserCollection::prepareUpdateName(UserPtr user)
{
    byNameUpdateIt_ = byName_.find(user->name());
}

void UserCollection::updateName(UserPtr user)
{
    if (byNameUpdateIt_ != byName_.end())
    {
        byName_.replace(byNameUpdateIt_, user);
    }
}

void UserCollection::prepareUpdateLastSeen(UserPtr user)
{
    byLastSeenUpdateIt_ = findInNonUniqueCollection(byLastSeen_, user, user->lastSeen());
}

void UserCollection::updateLastSeen(UserPtr user)
{
    if (byLastSeenUpdateIt_ != byLastSeen_.end())
    {
        byLastSeen_.replace(byLastSeenUpdateIt_, user);
    }
}

void UserCollection::prepareUpdateThreadCount(UserPtr user)
{
    byThreadCountUpdateIt_ = findInNonUniqueCollection(byThreadCount_, user, user->threadCount());
}

void UserCollection::updateThreadCount(UserPtr user)
{
    if (byThreadCountUpdateIt_ != byThreadCount_.end())
    {
        byThreadCount_.replace(byThreadCountUpdateIt_, user);
    }
}

void UserCollection::prepareUpdateMessageCount(UserPtr user)
{
    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, user, user->messageCount());
}

void UserCollection::updateMessageCount(UserPtr user)
{
    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        byMessageCount_.replace(byMessageCountUpdateIt_, user);
    }
}

void UserCollection::refreshByLastSeen()
{
    byLastSeen_.clear();
    for (UserPtr user : byId_)
    {
        byLastSeen_.insert(user);
    }
}

void UserCollection::refreshByThreadCount()
{
    byThreadCount_.clear();
    for (UserPtr user : byId_)
    {
        byThreadCount_.insert(user);
    }
}

void UserCollection::refreshByMessageCount()
{
    byMessageCount_.clear();
    for (UserPtr user : byId_)
    {
        byMessageCount_.insert(user);
    }
}
