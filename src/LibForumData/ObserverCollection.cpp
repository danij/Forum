#include <algorithm>

#include "ObserverCollection.h"

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

void ObserverCollection::addNewUser(PerformedByType performedBy, const Forum::Entities::User& newUser)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->addNewUser(performedBy, newUser);
}

void ObserverCollection::changeUser(PerformedByType performedBy, const Forum::Entities::User& user,
                                            Forum::Entities::User::ChangeType change)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->changeUser(performedBy, user, change);
}

void ObserverCollection::deleteUser(PerformedByType performedBy, const Forum::Entities::User& deletedUser)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->deleteUser(performedBy, deletedUser);
}


void ObserverCollection::getDiscussionThreadCount(const Entities::User& performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getDiscussionThreadCount(performedBy);
}

void ObserverCollection::getDiscussionThreads(const Entities::User& performedBy)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getDiscussionThreads(performedBy);
}

void ObserverCollection::addNewDiscussionThread(const Entities::User& performedBy,
                                                const Forum::Entities::DiscussionThread& newThread)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->addNewDiscussionThread(performedBy, newThread);
}

void ObserverCollection::getDiscussionThreadById(const Entities::User& performedBy, const Forum::Entities::IdType& id)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->getDiscussionThreadById(performedBy, id);
}

void ObserverCollection::changeDiscussionThread(const Entities::User& performedBy,
                                                const Forum::Entities::DiscussionThread& thread,
                                                Forum::Entities::DiscussionThread::ChangeType change)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->changeDiscussionThread(performedBy, thread, change);
}
