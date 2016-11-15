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


void ObserverCollection::onGetEntitiesCount(ObserverContext context)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetEntitiesCount(context);
}

void ObserverCollection::onGetUsers(ObserverContext context)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetUsers(context);
}

void ObserverCollection::onGetUserById(ObserverContext context, const IdType& id)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetUserById(context, id);
}

void ObserverCollection::onGetUserByName(ObserverContext context, const std::string& name)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetUserByName(context, name);
}

void ObserverCollection::onAddNewUser(ObserverContext context, const User& newUser)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onAddNewUser(context, newUser);
}

void ObserverCollection::onChangeUser(ObserverContext context, const User& user, User::ChangeType change)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onChangeUser(context, user, change);
}

void ObserverCollection::onDeleteUser(ObserverContext context, const User& deletedUser)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onDeleteUser(context, deletedUser);
}


void ObserverCollection::onGetDiscussionThreads(ObserverContext context)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetDiscussionThreads(context);
}

void ObserverCollection::onGetDiscussionThreadsOfUser(ObserverContext context, const User& user)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetDiscussionThreadsOfUser(context, user);
}

void ObserverCollection::onAddNewDiscussionThread(ObserverContext context, const DiscussionThread& newThread)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onAddNewDiscussionThread(context, newThread);
}

void ObserverCollection::onGetDiscussionThreadById(ObserverContext context, const IdType& id)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetDiscussionThreadById(context, id);
}

void ObserverCollection::onChangeDiscussionThread(ObserverContext context, const DiscussionThread& thread,
                                                DiscussionThread::ChangeType change)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onChangeDiscussionThread(context, thread, change);
}

void ObserverCollection::onDeleteDiscussionThread(ObserverContext context, const DiscussionThread& deletedThread)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onDeleteDiscussionThread(context, deletedThread);
}

void ObserverCollection::onAddNewDiscussionMessage(ObserverContext context, const DiscussionMessage& newMessage)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onAddNewDiscussionMessage(context, newMessage);
}

void ObserverCollection::onDeleteDiscussionMessage(ObserverContext context, const DiscussionMessage& deletedMessage)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : writeObservers_) item->onDeleteDiscussionMessage(context, deletedMessage);
}

void ObserverCollection::onGetDiscussionThreadMessagesOfUser(ObserverContext context, const User& user)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    for (auto& item : readObservers_) item->onGetDiscussionThreadMessagesOfUser(context, user);
}
