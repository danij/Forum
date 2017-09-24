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

MemoryRepositoryUser::MemoryRepositoryUser(MemoryStoreRef store, UserAuthorizationRef authorization,
                                           AuthorizationRepositoryRef authorizationRepository)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization)),
      authorizationRepository_(authorizationRepository)
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
        auto& currentUser = performedBy.get(collection, *store_);

        if ( ! (status = authorization_->getUsers(currentUser)))
        {
            return;
        }

        status.disable();

        auto pageSize = getGlobalConfig()->user.maxUsersPerPage;
        auto& displayContext = Context::getDisplayContext();

        SerializationRestriction restriction(collection.grantedPrivileges(), currentUser.id(), Context::getCurrentTime());

        auto ascending = displayContext.sortOrder == Context::SortOrder::Ascending;

        switch (by)
        {
        case RetrieveUsersBy::Name:
            writeEntitiesWithPagination(collection.users().byName(), "users", output, displayContext.pageNumber,
                pageSize, ascending, restriction);
            break;
        case RetrieveUsersBy::Created:
            writeEntitiesWithPagination(collection.users().byCreated(), "users", output, displayContext.pageNumber,
                pageSize, ascending, restriction);
            break;
        case RetrieveUsersBy::LastSeen:
            writeEntitiesWithPagination(collection.users().byLastSeen(), "users", output, displayContext.pageNumber,
                pageSize, ascending, restriction);
            break;
        case RetrieveUsersBy::ThreadCount:
            writeEntitiesWithPagination(collection.users().byThreadCount(), "users", output, displayContext.pageNumber,
                pageSize, ascending, restriction);
            break;
        case RetrieveUsersBy::MessageCount:
            writeEntitiesWithPagination(collection.users().byMessageCount(), "users", output, displayContext.pageNumber,
                pageSize, ascending, restriction);
            break;
        }

        readEvents().onGetUsers(createObserverContext(currentUser));
    });
    return status;
}

StatusCode MemoryRepositoryUser::getUserById(IdTypeRef id, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.users().byId();
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

                          SerializationRestriction restriction(collection.grantedPrivileges(), currentUser.id(),
                                                               Context::getCurrentTime());

                          writeSingleValueSafeName(output, "user", user, restriction);

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
                          auto& currentUser = performedBy.get(collection, *store_);

                          User::NameType nameString(name);

                          const auto& index = collection.users().byName();
                          auto it = index.find(nameString);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& user = **it;

                          if ( ! (status = authorization_->getUserByName(currentUser, **it)))
                          {
                              return;
                          }

                          status.disable();

                          SerializationRestriction restriction(collection.grantedPrivileges(), currentUser.id(),
                                                               Context::getCurrentTime());

                          writeSingleValueSafeName(output, "user", user, restriction);

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
    boost::optional<IdType> grantAllPrivilegesTo;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);
                           if ( ! (status = authorization_->addNewUser(*currentUser, name)))
                           {
                               return;
                           }

                           auto statusWithResource = addNewUser(collection, generateUniqueId(), name, auth);
                           auto& user = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           writeEvents().onAddNewUser(createObserverContext(*currentUser), *user);

                           if (1 == collection.users().count())
                           {
                               //this is the first user, so grant all privileges
                               grantAllPrivilegesTo = user->id();
                           }

                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", user->id());
                                               writer << Json::propertySafeName("name", user->name().string());
                                               writer << Json::propertySafeName("created", user->created());
                                           });
                       });
    if (grantAllPrivilegesTo)
    {
        Json::StringBuffer nullOutput;

        for (EnumIntType p = 0, n = static_cast<EnumIntType>(DiscussionThreadMessagePrivilege::COUNT); p < n; ++p)
        {
            auto privilege = static_cast<DiscussionThreadMessagePrivilege>(p);
            authorizationRepository_->assignDiscussionThreadMessagePrivilege(
                    *grantAllPrivilegesTo, privilege, MaxPrivilegeValue, UnlimitedDuration, nullOutput);
        }
        for (EnumIntType p = 0, n = static_cast<EnumIntType>(DiscussionThreadPrivilege::COUNT); p < n; ++p)
        {
            auto privilege = static_cast<DiscussionThreadPrivilege>(p);
            authorizationRepository_->assignDiscussionThreadPrivilege(
                    *grantAllPrivilegesTo, privilege, MaxPrivilegeValue, UnlimitedDuration, nullOutput);
        }
        for (EnumIntType p = 0, n = static_cast<EnumIntType>(DiscussionTagPrivilege::COUNT); p < n; ++p)
        {
            auto privilege = static_cast<DiscussionTagPrivilege>(p);
            authorizationRepository_->assignDiscussionTagPrivilege(
                    *grantAllPrivilegesTo, privilege, MaxPrivilegeValue, UnlimitedDuration, nullOutput);
        }
        for (EnumIntType p = 0, n = static_cast<EnumIntType>(DiscussionCategoryPrivilege::COUNT); p < n; ++p)
        {
            auto privilege = static_cast<DiscussionCategoryPrivilege>(p);
            authorizationRepository_->assignDiscussionCategoryPrivilege(
                    *grantAllPrivilegesTo, privilege, MaxPrivilegeValue, UnlimitedDuration, nullOutput);
        }
        for (EnumIntType p = 0, n = static_cast<EnumIntType>(ForumWidePrivilege::COUNT); p < n; ++p)
        {
            auto privilege = static_cast<ForumWidePrivilege>(p);
            authorizationRepository_->assignForumWidePrivilege(
                    *grantAllPrivilegesTo, privilege, MaxPrivilegeValue, UnlimitedDuration, nullOutput);
        }
    }
    return status;
}

StatusWithResource<UserPtr> MemoryRepositoryUser::addNewUser(EntityCollection& collection, IdTypeRef id,
                                                             StringView name, StringView auth)
{
    auto authString = toString(auth);

    auto& indexByAuth = collection.users().byAuth();
    if (indexByAuth.find(authString) != indexByAuth.end())
    {
        return StatusCode::USER_WITH_SAME_AUTH_ALREADY_EXISTS;
    }

    User::NameType nameString(name);

    auto& indexByName = collection.users().byName();
    if (indexByName.find(nameString) != indexByName.end())
    {
        return StatusCode::ALREADY_EXISTS;
    }

    auto user = collection.createUser(id, std::move(nameString), Context::getCurrentTime(),
                                      { Context::getCurrentUserIpAddress() });
    user->updateAuth(std::move(authString));

    collection.insertUser(user);

    return user;
}

StatusCode MemoryRepositoryUser::changeUserName(IdTypeRef id, StringView newName, OutStream& output)
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
                           auto& indexById = collection.users().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& user = **it;

                           if ( ! (status = authorization_->changeUserName(*currentUser, user, newName)))
                           {
                               return;
                           }

                           if ( ! (status = changeUserName(collection, id, newName))) return;

                           writeEvents().onChangeUser(createObserverContext(*currentUser), user, User::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserName(EntityCollection& collection, IdTypeRef id, StringView newName)
{
    auto& indexById = collection.users().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    User::NameType newNameString(newName);

    auto& indexByName = collection.users().byName();
    if (indexByName.find(newNameString) != indexByName.end())
    {
        return StatusCode::ALREADY_EXISTS;
    }

    UserPtr user = *it;
    user->updateName(std::move(newNameString));

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::changeUserInfo(IdTypeRef id, StringView newInfo, OutStream& output)
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
                           auto& indexById = collection.users().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& user = **it;

                           if ( ! (status = authorization_->changeUserInfo(*currentUser, user, newInfo)))
                           {
                               return;
                           }

                           if ( ! (status = changeUserInfo(collection, id, newInfo))) return;

                           writeEvents().onChangeUser(createObserverContext(*currentUser), user, User::ChangeType::Info);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserInfo(EntityCollection& collection, IdTypeRef id, StringView newInfo)
{
    auto& indexById = collection.users().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->info() = User::InfoType(newInfo);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::deleteUser(IdTypeRef id, OutStream& output)
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
                           auto& indexById = collection.users().byId();
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

                           status = deleteUser(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::deleteUser(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.users().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    collection.deleteUser(*it);

    return StatusCode::OK;
}
