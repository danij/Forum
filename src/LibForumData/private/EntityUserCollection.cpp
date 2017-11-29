/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include "EntityUserCollection.h"
#include "ContextProviders.h"

using namespace Forum::Entities;

bool UserCollection::add(UserPtr user)
{
    if ( ! std::get<1>(byId_.insert(user))) return false;
    byAuth_.insert(user);
    byName_.insert(user);
    byCreated_.insert(user);

    if ( ! Context::isBatchInsertInProgress())
    {
        byLastSeen_.insert(user);
        byThreadCount_.insert(user);
        byMessageCount_.insert(user);
    }

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

    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byLastSeen_, user, user->lastSeen());
        eraseFromNonUniqueCollection(byThreadCount_, user, user->threadCount());
        eraseFromNonUniqueCollection(byMessageCount_, user, user->messageCount());
    }

    return true;
}

void UserCollection::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byLastSeen_.clear();
    for (UserPtr user : byId_)
    {
        byLastSeen_.insert(user);
    }

    byThreadCount_.clear();
    for (UserPtr user : byId_)
    {
        byThreadCount_.insert(user);
    }

    byMessageCount_.clear();
    for (UserPtr user : byId_)
    {
        byMessageCount_.insert(user);
    }
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
    if (Context::isBatchInsertInProgress()) return;

    byLastSeenUpdateIt_ = findInNonUniqueCollection(byLastSeen_, user, user->lastSeen());
}

void UserCollection::updateLastSeen(UserPtr user)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byLastSeenUpdateIt_ != byLastSeen_.end())
    {
        byLastSeen_.replace(byLastSeenUpdateIt_, user);
    }
}

void UserCollection::prepareUpdateThreadCount(UserPtr user)
{
    if (Context::isBatchInsertInProgress()) return;

    byThreadCountUpdateIt_ = findInNonUniqueCollection(byThreadCount_, user, user->threadCount());
}

void UserCollection::updateThreadCount(UserPtr user)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byThreadCountUpdateIt_ != byThreadCount_.end())
    {
        byThreadCount_.replace(byThreadCountUpdateIt_, user);
    }
}

void UserCollection::prepareUpdateMessageCount(UserPtr user)
{
    if (Context::isBatchInsertInProgress()) return;

    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, user, user->messageCount());
}

void UserCollection::updateMessageCount(UserPtr user)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        byMessageCount_.replace(byMessageCountUpdateIt_, user);
    }
}
