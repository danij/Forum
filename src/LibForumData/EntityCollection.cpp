#include "EntityCollection.h"

using namespace Forum::Entities;

const User Forum::Entities::AnonymousUser("<anonymous>");

void EntityCollection::modifyUser(const IdType& id, std::function<void(User&)> modifyFunction)
{
    auto& index = users_.get<UserCollectionById>();
    auto it = index.find(id);
    if (it == index.end())
    {
        return;
    }
    users_.modify(it, [&modifyFunction](const UserRef& user)
    {
       if (user)
       {
           modifyFunction(*user);
       }
    });
}

void EntityCollection::deleteUser(const IdType& id)
{
    auto& index = users_.get<UserCollectionById>();
    auto it = index.find(id);
    if (it == index.end())
    {
        return;
    }
    users_.erase(it);
}

void EntityCollection::modifyDiscussionThread(const IdType& id, std::function<void(DiscussionThread&)> modifyFunction)
{
    auto& index = threads_.get<DiscussionThreadCollectionById>();
    auto it = index.find(id);
    if (it == index.end())
    {
        return;
    }
    threads_.modify(it, [&modifyFunction](const DiscussionThreadRef& thread)
    {
        if (thread)
        {
            modifyFunction(*thread);
        }
    });
}

void EntityCollection::deleteDiscussionThread(const IdType& id)
{
    auto& index = threads_.get<DiscussionThreadCollectionById>();
    auto it = index.find(id);
    if (it == index.end())
    {
        return;
    }
    threads_.erase(it);
}
