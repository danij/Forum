#include "MemoryRepositoryUser.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"

#include <unicode/ustring.h>
#include <unicode/uchar.h>

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

static constexpr size_t MaxNrOfUserNameChars16 = 65536;
static constexpr size_t MaxNrOfUserNameChars32 = MaxNrOfUserNameChars16 / 2;
static thread_local std::unique_ptr<UChar[]> validUserNameBuffer16(new UChar[MaxNrOfUserNameChars16]);
static thread_local std::unique_ptr<UChar32[]> validUserNameBuffer32(new UChar32[MaxNrOfUserNameChars32]);

static bool isValidUserName(StringView input)
{
    //"^[[:alnum:]]+[ _-]*[[:alnum:]]+$"
    if (input.size() < 1)
    {
        return false;
    }

    int32_t written;
    UErrorCode errorCode{};

    auto u16Chars = u_strFromUTF8Lenient(validUserNameBuffer16.get(), MaxNrOfUserNameChars16, &written, 
                                         input.data(), input.size(), &errorCode);
    if (U_FAILURE(errorCode)) return false;

    errorCode = {};
    auto u32Chars = u_strToUTF32(validUserNameBuffer32.get(), MaxNrOfUserNameChars32, &written, 
                                 u16Chars, written, &errorCode);
    if (U_FAILURE(errorCode)) return false;

    if ((u_isalnum(u32Chars[0]) == FALSE) || (u_isalnum(u32Chars[written - 1]) == FALSE))
    {
        return false;
    }

    for (int i = 1; i < written - 1; ++i)
    {
        if (' ' == u32Chars[i]) continue;
        if ('_' == u32Chars[i]) continue;
        if ('-' == u32Chars[i]) continue;
        if (u_isalnum(u32Chars[i])) continue;

        return false;
    }

    return true;
}

MemoryRepositoryUser::MemoryRepositoryUser(MemoryStoreRef store, UserAuthorizationRef authorization) 
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MemoryRepositoryUser::getUsers(OutStream& output, RetrieveUsersBy by) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, store());

        if ( ! (status = authorization_->getUsers(currentUser)))
        {
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

        readEvents().onGetUsers(createObserverContext(currentUser));
    });
    return status;
}

StatusCode MemoryRepositoryUser::getUserById(const IdType& id, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

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

                          auto& user = **it;

                          if ( ! (status = authorization_->getUserById(currentUser, user)))
                          {
                              return;
                          }

                          status.disable();
                          writeSingleValueSafeName(output, "user", user);
                          
                          readEvents().onGetUserById(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getUserByName(StringView name, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    if (countUTF8Characters(name) > getGlobalConfig()->user.maxNameLength)
    {
        return status = INVALID_PARAMETERS;
    }
    
    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());

                          const auto& index = collection.usersByName();
                          auto it = index.find(name);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          if ( ! (status = authorization_->getUserByName(currentUser, **it)))
                          {
                              return;
                          }

                          status.disable();
                          writeSingleValueSafeName(output, "user", **it);

                          readEvents().onGetUserByName(createObserverContext(currentUser), name);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::addNewUser(StringView name, StringView auth, OutStream& output)
{
    StatusWriter status(output);

    if (auth.size() < 1)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto config = getGlobalConfig();
    auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING, 
                                         config->user.minNameLength, config->user.maxNameLength,
                                         isValidUserName);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto authString = toString(auth);

                           auto& indexByAuth = collection.users().get<EntityCollection::UserCollectionByAuth>();
                           if (indexByAuth.find(authString) != indexByAuth.end())
                           {
                               status = StatusCode::USER_WITH_SAME_AUTH_ALREADY_EXISTS;
                               return;
                           }

                           auto nameString = toString(name);

                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                           if (indexByName.find(nameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           if ( ! (status = authorization_->addNewUser(*currentUser, name)))
                           {
                               return;
                           }

                           auto user = std::make_shared<User>();
                           user->id() = generateUUIDString();
                           user->auth() = std::move(authString);
                           user->name() = std::move(nameString);
                           updateCreated(*user);

                           collection.users().insert(user);

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

StatusCode MemoryRepositoryUser::changeUserName(const IdType& id, StringView newName, OutStream& output)
{
    StatusWriter status(output);
    auto config = getGlobalConfig();
    auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->user.minNameLength, config->user.maxNameLength,
                                         isValidUserName);

    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

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

                           if ( ! (status = authorization_->changeUserName(*currentUser, **it, newName)))
                           {
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
                           writeEvents().onChangeUser(createObserverContext(*currentUser), **it, User::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserInfo(const IdType& id, StringView newInfo, OutStream& output)
{
    StatusWriter status(output);

    auto config = getGlobalConfig();
    auto validationCode = validateString(newInfo, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->user.minInfoLength, config->user.maxInfoLength);

    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

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

                           if ( ! (status = authorization_->changeUserInfo(*currentUser, **it, newInfo)))
                           {
                               return;
                           }

                           collection.modifyUser(it, [&newInfo](User& user)
                                                     {
                                                         user.info() = toString(newInfo);
                                                     });

                           writeEvents().onChangeUser(createObserverContext(*currentUser), **it, User::ChangeType::Info);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::deleteUser(const IdType& id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

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

                           if ( ! (status = authorization_->deleteUser(*currentUser, **it)))
                           {
                               return;
                           }

                           //make sure the user is not deleted before being passed to the observers
                           writeEvents().onDeleteUser(createObserverContext(*currentUser), **it);

                           collection.deleteUser(it);
                       });
    return status;
}
