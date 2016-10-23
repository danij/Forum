#include <algorithm>

#include "ObserverCollection.h"

using namespace Forum::Entities;
using namespace Forum::Repository;

template <typename T, typename TCollection>
inline bool contains(const TCollection& collection, const T& value)
{
    auto collectionEnd = std::cend(collection);
    return std::find(std::cbegin(collection), collectionEnd, value) != collectionEnd;
};

template <typename T, typename TCollection>
inline void remove(TCollection& collection, const T& value)
{
    auto collectionEnd = std::cend(collection);
    auto it = std::find(std::cbegin(collection), collectionEnd, value);
    if (it != collectionEnd)
    {
        collection.erase(it);
    }
};

void ObserverCollection::addObserver(const ReadRepositoryObserverRef& observer)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    if (contains(readObservers_, observer)) return;
    readObservers_.push_back(observer);
}

void ObserverCollection::addObserver(const WriteRepositoryObserverRef& observer)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    if (contains(writeObservers_, observer)) return;
    writeObservers_.push_back(observer);
}

void ObserverCollection::removeObserver(const ReadRepositoryObserverRef& observer)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    remove(readObservers_, observer);
}

void ObserverCollection::removeObserver(const WriteRepositoryObserverRef& observer)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    remove(writeObservers_, observer);
}


void ObserverCollection::onGetUserCount(PerformedByType performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetUserCount(performedBy);
}

void ObserverCollection::onGetUsers(PerformedByType performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetUsers(performedBy);
}

void ObserverCollection::onGetUserById(PerformedByType performedBy, const IdType& id)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetUserById(performedBy, id);
}

void ObserverCollection::onGetUserByName(PerformedByType performedBy, const std::string& name)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetUserByName(performedBy, name);
}

void ObserverCollection::onAddNewUser(PerformedByType performedBy, const User& newUser)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onAddNewUser(performedBy, newUser);
}

void ObserverCollection::onChangeUser(PerformedByType performedBy, const User& user, User::ChangeType change)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onChangeUser(performedBy, user, change);
}

void ObserverCollection::onDeleteUser(PerformedByType performedBy, const User& deletedUser)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onDeleteUser(performedBy, deletedUser);
}


void ObserverCollection::onGetDiscussionThreadCount(PerformedByType performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetDiscussionThreadCount(performedBy);
}

void ObserverCollection::onGetDiscussionThreads(PerformedByType performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetDiscussionThreads(performedBy);
}

void ObserverCollection::onGetDiscussionThreadsOfUser(PerformedByType performedBy, const User& user)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetDiscussionThreadsOfUser(performedBy, user);
}

void ObserverCollection::onAddNewDiscussionThread(PerformedByType performedBy, const DiscussionThread& newThread)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onAddNewDiscussionThread(performedBy, newThread);
}

void ObserverCollection::onGetDiscussionThreadById(PerformedByType performedBy, const IdType& id)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetDiscussionThreadById(performedBy, id);
}

void ObserverCollection::onChangeDiscussionThread(PerformedByType performedBy, const DiscussionThread& thread,
                                                DiscussionThread::ChangeType change)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onChangeDiscussionThread(performedBy, thread, change);
}

void ObserverCollection::onDeleteDiscussionThread(PerformedByType performedBy, const DiscussionThread& deletedThread)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onDeleteDiscussionThread(performedBy, deletedThread);
}
