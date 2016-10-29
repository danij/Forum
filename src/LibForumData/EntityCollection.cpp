#include "EntityCollection.h"
#include "StateHelpers.h"

using namespace Forum::Entities;
using namespace Forum::Helpers;

const UserRef Forum::Entities::AnonymousUser = std::make_shared<User>("<anonymous>");

void EntityCollection::modifyUser(UserCollection::iterator iterator, std::function<void(User&)> modifyFunction)
{
    if (iterator == users_.end())
    {
        return;
    }
    users_.modify(iterator, [&modifyFunction](const UserRef& user)
    {
        if (user)
        {
            modifyFunction(*user);
        }
    });
}

void EntityCollection::modifyUser(const IdType& id, std::function<void(User&)> modifyFunction)
{
    return modifyUser(users_.get<UserCollectionById>().find(id), modifyFunction);
}

/**
 * Used to prevent the individual removal of threads from a user's created threads collection when deleting a user
 */
static thread_local bool alsoDeleteThreadsFromUser = true;

void EntityCollection::deleteUser(UserCollection::iterator iterator)
{
    if (iterator == users_.end())
    {
        return;
    }
    BoolTemporaryChanger changer(alsoDeleteThreadsFromUser, false);
    for (auto& thread : (*iterator)->threads())
    {
        static_cast<DiscussionThreadCollectionBase*>(this)->deleteDiscussionThread(thread->id());
    }
    users_.erase(iterator);
}

void EntityCollection::deleteUser(const IdType& id)
{
    deleteUser(users_.get<UserCollectionById>().find(id));
}

void DiscussionThreadCollectionBase::modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                                            std::function<void(DiscussionThread&)> modifyFunction)
{
    if (iterator == threads_.end())
    {
        return;
    }
    threads_.modify(iterator, [&modifyFunction](const DiscussionThreadRef& thread)
    {
        if (thread)
        {
            modifyFunction(*thread);
        }
    });
}

void EntityCollection::modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                              std::function<void(DiscussionThread&)> modifyFunction)
{
    if (iterator == threads_.end())
    {
        return;
    }
    threads_.modify(iterator, [&modifyFunction](const DiscussionThreadRef& thread)
    {
        if (thread)
        {
            auto user = thread->createdBy().lock();
            if (user && user->id())
            {
                user->modifyDiscussionThread(thread->id(), modifyFunction);
            }
            else
            {
                modifyFunction(*thread);
            }
        }
    });
}

void DiscussionThreadCollectionBase::modifyDiscussionThread(const IdType& id,
                                                            std::function<void(DiscussionThread&)> modifyFunction)
{
    modifyDiscussionThread(threads_.get<DiscussionThreadCollectionById>().find(id), modifyFunction);
}

void DiscussionThreadCollectionBase::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    if (iterator == threads_.end())
    {
        return;
    }
    threads_.erase(iterator);
}

void EntityCollection::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    if (iterator == threads_.end())
    {
        return;
    }
    if (alsoDeleteThreadsFromUser)
    {
        auto user = (*iterator)->createdBy().lock();
        if (user && user->id())
        {
            user->deleteDiscussionThread((*iterator)->id());
        }
    }
    threads_.erase(iterator);
}

void DiscussionThreadCollectionBase::deleteDiscussionThread(const IdType& id)
{
    deleteDiscussionThread(threads_.get<DiscussionThreadCollectionById>().find(id));
}
