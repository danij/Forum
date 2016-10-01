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
