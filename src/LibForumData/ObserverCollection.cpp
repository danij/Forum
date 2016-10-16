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


void ObserverCollection::getUserCount(PerformedByType performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getUserCount(performedBy);
}

void ObserverCollection::getUsers(PerformedByType performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getUsers(performedBy);
}

void ObserverCollection::getUserByName(PerformedByType performedBy, const std::string& name)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getUserByName(performedBy, name);
}

void ObserverCollection::addNewUser(PerformedByType performedBy, const User& newUser)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->addNewUser(performedBy, newUser);
}

void ObserverCollection::changeUser(PerformedByType performedBy, const User& user, User::ChangeType change)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->changeUser(performedBy, user, change);
}

void ObserverCollection::deleteUser(PerformedByType performedBy, const User& deletedUser)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->deleteUser(performedBy, deletedUser);
}


void ObserverCollection::getDiscussionThreadCount(PerformedByType performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getDiscussionThreadCount(performedBy);
}

void ObserverCollection::getDiscussionThreads(PerformedByType performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getDiscussionThreads(performedBy);
}

void ObserverCollection::addNewDiscussionThread(PerformedByType performedBy, const DiscussionThread& newThread)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->addNewDiscussionThread(performedBy, newThread);
}

void ObserverCollection::getDiscussionThreadById(PerformedByType performedBy, const IdType& id)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getDiscussionThreadById(performedBy, id);
}

void ObserverCollection::changeDiscussionThread(PerformedByType performedBy, const DiscussionThread& thread,
                                                DiscussionThread::ChangeType change)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->changeDiscussionThread(performedBy, thread, change);
}
