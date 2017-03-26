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
using namespace Forum::Authorization;

MemoryRepositoryUser::MemoryRepositoryUser(MemoryStoreRef store, UserAuthorizationRef authorization) 
    : MemoryRepositoryBase(std::move(store)), validUserNameRegex(boost::make_u32regex("^[[:alnum:]]+[ _-]*[[:alnum:]]+$")),
    authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MemoryRepositoryUser::getUsers(OutStream& output, RetrieveUsersBy by) const
{
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, store());

        authorizationStatus = authorization_->getUsers(currentUser);
        if (AuthorizationStatusCode::OK != authorizationStatus.code)
        {
            status = authorizationStatus.code;
            return;
        }

        status.disable();

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
            readEvents().onGetUsers(createObserverContext(currentUser));
    });
    return status;
}

StatusCode MemoryRepositoryUser::getUserById(const IdType& id, OutStream& output) const
{
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());

                          const auto& index = collection.usersById();
                          auto it = index.find(id);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          authorizationStatus = authorization_->getUserById(currentUser, **it);
                          if (AuthorizationStatusCode::OK != authorizationStatus.code)
                          {
                              status = authorizationStatus.code;
                              return;
                          }

                          status.disable();
                          writeSingleValueSafeName(output, "user", **it);
                          
                          if ( ! Context::skipObservers())
                              readEvents().onGetUserById(createObserverContext(currentUser), id);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getUserByName(const StringView& name, OutStream& output) const
{
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    //keep a string around in order to avoid dynamic memory allocation on each call
    //when converting from StringView to std::string in order to use index.find();

    static thread_local std::string nameString;

    if (countUTF8Characters(name) > getGlobalConfig()->user.maxNameLength)
    {
        return status = INVALID_PARAMETERS;
    }
    
    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());

                          const auto& index = collection.usersByName();
                          auto it = index.find(toString(name, nameString));
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          authorizationStatus = authorization_->getUserByName(currentUser, **it);
                          if (AuthorizationStatusCode::OK != authorizationStatus.code)
                          {
                              status = authorizationStatus.code;
                              return;
                          }

                          status.disable();
                          writeSingleValueSafeName(output, "user", **it);

                          if ( ! Context::skipObservers())
                              readEvents().onGetUserByName(createObserverContext(currentUser), name);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::addNewUser(const StringView& name, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);

    auto config = getGlobalConfig();
    auto validationCode = validateString(name, validUserNameRegex, INVALID_PARAMETERS_FOR_EMPTY_STRING, 
                                         config->user.minNameLength, config->user.maxNameLength);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto nameString = toString(name);

                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                           if (indexByName.find(nameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           authorizationStatus = authorization_->addNewUser(*currentUser, name);
                           if (AuthorizationStatusCode::OK != authorizationStatus.code)
                           {
                               status = authorizationStatus.code;
                               return;
                           }

                           auto user = std::make_shared<User>();
                           user->id() = generateUUIDString();
                           user->name() = std::move(nameString);
                           updateCreated(*user);

                           collection.users().insert(user);

                           if ( ! Context::skipObservers())
                               writeEvents().onAddNewUser(createObserverContext(*currentUser), *user);

                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", user->id());
                                               writer << Json::propertySafeName("name", user->name());
                                               writer << Json::propertySafeName("created", user->created());
                                           });
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserName(const IdType& id, const StringView& newName, OutStream& output)
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
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           authorizationStatus = authorization_->changeUserName(*currentUser, **it, newName);
                           if (AuthorizationStatusCode::OK != authorizationStatus.code)
                           {
                               status = authorizationStatus.code;
                               return;
                           }

                           auto newNameString = toString(newName);

                           auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                           if (indexByName.find(newNameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                           collection.modifyUser(it, [&newNameString](User& user)
                                                     {
                                                         user.name() = std::move(newNameString);
                                                     });
                           if ( ! Context::skipObservers())
                               writeEvents().onChangeUser(createObserverContext(*currentUser),
                                                          **it, User::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserInfo(const IdType& id, const StringView& newInfo, OutStream& output)
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
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           authorizationStatus = authorization_->changeUserInfo(*currentUser, **it, newInfo);
                           if (AuthorizationStatusCode::OK != authorizationStatus.code)
                           {
                               status = authorizationStatus.code;
                               return;
                           }

                           collection.modifyUser(it, [&newInfo](User& user)
                                                     {
                                                         user.info() = toString(newInfo);
                                                     });

                           if ( ! Context::skipObservers())
                               writeEvents().onChangeUser(createObserverContext(*currentUser),
                                                          **it, User::ChangeType::Info);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::deleteUser(const IdType& id, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           authorizationStatus = authorization_->deleteUser(*currentUser, **it);
                           if (AuthorizationStatusCode::OK != authorizationStatus.code)
                           {
                               status = authorizationStatus.code;
                               return;
                           }

                           //make sure the user is not deleted before being passed to the observers
                           if ( ! Context::skipObservers())
                               writeEvents().onDeleteUser(createObserverContext(*currentUser), **it);

                           collection.deleteUser(it);
                       });
    return status;
}
