#include "EntityCollection.h"

using namespace Forum::Entities;

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

void EntityCollection::deleteUser(UserCollection::iterator iterator)
{
    if (iterator == users_.end())
    {
        return;
    }
    users_.erase(iterator);
}

void EntityCollection::deleteUser(const IdType& id)
{
    deleteUser(users_.get<UserCollectionById>().find(id));
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
            modifyFunction(*thread);
        }
    });
}

void EntityCollection::modifyDiscussionThread(const IdType& id, std::function<void(DiscussionThread&)> modifyFunction)
{
    modifyDiscussionThread(threads_.get<DiscussionThreadCollectionById>().find(id), modifyFunction);
}

void EntityCollection::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    if (iterator == threads_.end())
    {
        return;
    }
    threads_.erase(iterator);
}

void EntityCollection::deleteDiscussionThread(const IdType& id)
{
    deleteDiscussionThread(threads_.get<DiscussionThreadCollectionById>().find(id));
}
