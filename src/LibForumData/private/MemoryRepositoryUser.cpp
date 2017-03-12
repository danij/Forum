#include "MemoryRepositoryUser.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

MemoryRepositoryUser::MemoryRepositoryUser(MemoryStoreRef store) : MemoryRepositoryBase(std::move(store)),
    validUserNameRegex(boost::make_u32regex("^[[:alnum:]]+[ _-]*[[:alnum:]]+$"))
{}

StatusCode MemoryRepositoryUser::getUsers(std::ostream& output, RetrieveUsersBy by) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto pageSize = getGlobalConfig()->user.maxUsersPerPage;
        auto& displayContext = Context::getDisplayContext();

        switch (by)
        {
        case RetrieveUsersBy::Name:
            writeEntitiesWithPagination(collection.usersByName(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](const auto& u) { return u; });
            break;
        case RetrieveUsersBy::Created:
            writeEntitiesWithPagination(collection.usersByCreated(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](const auto& u) { return u; });
            break;
        case RetrieveUsersBy::LastSeen:
            writeEntitiesWithPagination(collection.usersByLastSeen(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](const auto& u) { return u; });
            break;
        case RetrieveUsersBy::ThreadCount:
            writeEntitiesWithPagination(collection.usersByThreadCount(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](const auto& u) { return u; });
            break;
        case RetrieveUsersBy::MessageCount:
            writeEntitiesWithPagination(collection.usersByMessageCount(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](const auto& u) { return u; });
            break;
        }

        if ( ! Context::skipObservers())
            readEvents().onGetUsers(createObserverContext(performedBy.get(collection, store())));
    });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::getUserById(const IdType& id, std::ostream& output) const
{
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          const auto& index = collection.usersById();
                          auto it = index.find(id);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          else
                          {
                              status.disable();
                              writeSingleValueSafeName(output, "user", **it);
                          }
                          if ( ! Context::skipObservers())
                              readEvents().onGetUserById(createObserverContext(performedBy.get(collection, store())), id);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getUserByName(const std::string& name, std::ostream& output) const
{
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          const auto& index = collection.usersByName();
                          auto it = index.find(name);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          else
                          {
                              status.disable();
                              writeSingleValueSafeName(output, "user", **it);
                          }
                          if ( ! Context::skipObservers())
                              readEvents().onGetUserByName(createObserverContext(performedBy.get(collection, store())),
                                                           name);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::addNewUser(const std::string& name, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);

    auto config = getGlobalConfig();
    auto validationCode = validateString(name, validUserNameRegex, INVALID_PARAMETERS_FOR_EMPTY_STRING, 
                                         config->user.minNameLength, config->user.maxNameLength);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    auto user = std::make_shared<User>();
    user->id() = generateUUIDString();
    user->name() = name;
    updateCreated(*user);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                           if (indexByName.find(name) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                           collection.users().insert(user);

                           if ( ! Context::skipObservers())
                               writeEvents().onAddNewUser(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                          *user);
                 
                           status.addExtraSafeName("id", user->id());
                           status.addExtraSafeName("name", user->name());
                           status.addExtraSafeName("created", user->created());
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto config = getGlobalConfig();
    auto validationCode = validateString(newName, validUserNameRegex, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->user.minNameLength, config->user.maxNameLength);

    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                           if (indexByName.find(newName) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                           collection.modifyUser(it, [&newName](User& user)
                                                     {
                                                         user.name() = newName;
                                                     });
                           if ( ! Context::skipObservers())
                               writeEvents().onChangeUser(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                          **it, User::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserInfo(const IdType& id, const std::string& newInfo, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);

    auto config = getGlobalConfig();
    auto validationCode = validateString(newInfo, boost::none, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->user.minInfoLength, config->user.maxInfoLength);

    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           collection.modifyUser(it, [&newInfo](User& user)
                                                     {
                                                         user.info() = newInfo;
                                                     });

                           if ( ! Context::skipObservers())
                               writeEvents().onChangeUser(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                          **it, User::ChangeType::Info);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::deleteUser(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           //make sure the user is not deleted before being passed to the observers
                           if ( ! Context::skipObservers())
                               writeEvents().onDeleteUser(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                          **it);

                           collection.deleteUser(it);
                       });
    return status;
}
