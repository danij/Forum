/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MemoryRepositoryUser.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"
#include "Logging.h"

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

StatusCode MemoryRepositoryUser::getUsersOnline(OutStream& output) const
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

                          SerializationRestriction restriction(collection.grantedPrivileges(), currentUser.id(),
                                                               Context::getCurrentTime());

                          auto onlineUsersIntervalSeconds = getGlobalConfig()->user.onlineUsersIntervalSeconds;
                          auto onlineUsersTimeLimit =
                                  static_cast<Timestamp>(Context::getCurrentTime() - onlineUsersIntervalSeconds);

                          Json::JsonWriter writer(output);

                          writer.startObject();
                          writer.newPropertyWithSafeName("online_users");

                          writer.startArray();

                          auto index = collection.users().byLastSeen();
                          for (auto it = index.rbegin(), itEnd = index.rend(); it != itEnd; ++it)
                          {
                              const User* userPtr = *it;
                              assert(userPtr);

                              if (userPtr->lastSeen() < onlineUsersTimeLimit)
                              {
                                  break;
                              }
                              else
                              {
                                  serialize(writer, *userPtr, restriction);
                              }
                          }

                          writer.endArray();
                          writer.endObject();

                          readEvents().onGetUsersOnline(createObserverContext(currentUser));
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

                          readEvents().onGetUserByName(createObserverContext(currentUser), name);

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
                      });
    return status;
}

StatusCode MemoryRepositoryUser::searchUsersByName(StringView name, OutStream& output) const
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

                          readEvents().onSearchUsersByName(createObserverContext(currentUser), name);

                          User::NameType nameString(name);

                          const auto& index = collection.users().byName();
                          auto boundIndex = index.lower_bound_rank(nameString);
                          if (boundIndex >= index.size())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          status = StatusCode::OK;

                          SerializationRestriction restriction(collection.grantedPrivileges(), currentUser.id(),
                                                               Context::getCurrentTime());

                          status.writeNow([&](auto& writer)
                                          {
                                              writer << Json::propertySafeName("index", boundIndex);
                                              writer << Json::propertySafeName("pageSize", getGlobalConfig()->user.maxUsersPerPage);
                                          });
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getUserLogo(IdTypeRef id, OutStream& output) const
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

                          if ( ! user.hasLogo())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          status.disable();

                          const auto& logo = user.logo();
                          output.write(logo.data(), logo.size());

                          readEvents().onGetUserLogo(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getUserVoteHistory(IdTypeRef id, OutStream& output) const
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

                          if ( ! (status = authorization_->getUserVoteHistory(currentUser, user)))
                          {
                              return;
                          }

                          status.disable();

                          Json::JsonWriter writer(output);
                          writer.startObject();
                          writer.newPropertyWithSafeName("lastRetrievedAt")
                                  << user.voteHistoryLastRetrieved().exchange(static_cast<int64_t>(Context::getCurrentTime()));

                          writer.newPropertyWithSafeName("receivedVotes");
                          writer.startArray();

                          const auto& messageIndex = collection.threadMessages().byId();

                          for (const User::ReceivedVoteHistory& entry : user.voteHistory())
                          {
                              writer.startObject();

                              auto messageIt = messageIndex.find(entry.discussionThreadMessageId);
                              if (messageIt != messageIndex.end())
                              {
                                  auto& message = **messageIt;
                                  writer.newPropertyWithSafeName("messageId") << message.id();

                                  auto parentThread = message.parentThread();
                                  assert(parentThread);

                                  auto messageRank = parentThread->messages().findRankByCreated(message.id());
                                  if (messageRank)
                                  {
                                      writer.newPropertyWithSafeName("messageRank") << *messageRank;
                                  }

                                  writer.newPropertyWithSafeName("threadId") << parentThread->id();
                                  writer.newPropertyWithSafeName("threadName") << parentThread->name();
                              }

                              auto userIt = index.find(entry.voterId);
                              if (userIt != index.end())
                              {
                                  auto& voter = **userIt;
                                  writer.newPropertyWithSafeName("voterId") << voter.id();
                                  writer.newPropertyWithSafeName("voterName") << voter.name();
                              }

                              writer.newPropertyWithSafeName("at") << entry.at;

                              writer.newPropertyWithSafeName("type");

                              switch (entry.type)
                              {
                              case User::ReceivedVoteHistoryEntryType::UpVote:
                                  writer.writeSafeString("up");
                                  break;
                              case User::ReceivedVoteHistoryEntryType::DownVote:
                                  writer.writeSafeString("down");
                                  break;
                              case User::ReceivedVoteHistoryEntryType::ResetVote:
                                  writer.writeSafeString("reset");
                                  break;
                              }

                              writer.endObject();
                          }

                          writer.endArray();
                          writer.endObject();

                          readEvents().onGetUserVoteHistory(createObserverContext(currentUser), user);
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

                           User::NameType nameString(name);
                           auto& indexByName = collection.users().byName();
                           if (indexByName.find(nameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           auto statusWithResource = addNewUser(collection, generateUniqueId(), std::move(nameString),
                                                                auth);
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

        authorizationRepository_->assignForumWidePrivilege(*grantAllPrivilegesTo, MaxPrivilegeValue, UnlimitedDuration,
                                                           nullOutput);
    }
    return status;
}

StatusWithResource<UserPtr> MemoryRepositoryUser::addNewUser(EntityCollection& collection, IdTypeRef id,
                                                             StringView name, StringView auth)
{
    return addNewUser(collection, id, User::NameType(name), auth);
}

StatusWithResource<UserPtr> MemoryRepositoryUser::addNewUser(EntityCollection& collection, IdTypeRef id,
                                                             User::NameType&& name, StringView auth)
{
    auto authString = toString(auth);

    auto& indexByAuth = collection.users().byAuth();
    if (indexByAuth.find(authString) != indexByAuth.end())
    {
        FORUM_LOG_ERROR << "A user with this auth already exists: " << auth;
        return StatusCode::USER_WITH_SAME_AUTH_ALREADY_EXISTS;
    }

    auto& indexByName = collection.users().byName();
    if (indexByName.find(name) != indexByName.end())
    {
        FORUM_LOG_ERROR << "A user with this name already exists: " << name.string();
        return StatusCode::ALREADY_EXISTS;
    }

    auto user = collection.createUser(id, std::move(name), Context::getCurrentTime(),
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

                           User::NameType newNameString(newName);

                           auto& indexByName = collection.users().byName();
                           if (indexByName.find(newNameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           if ( ! (status = authorization_->changeUserName(*currentUser, user, newName)))
                           {
                               return;
                           }

                           if ( ! (status = changeUserName(collection, id, std::move(newNameString)))) return;

                           writeEvents().onChangeUser(createObserverContext(*currentUser), user, User::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserName(EntityCollection& collection, IdTypeRef id, StringView newName)
{
    return changeUserName(collection, id, User::NameType(newName));
}

StatusCode MemoryRepositoryUser::changeUserName(EntityCollection& collection, IdTypeRef id, User::NameType&& newName)
{
    auto& indexById = collection.users().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    auto& indexByName = collection.users().byName();
    if (indexByName.find(newName) != indexByName.end())
    {
        FORUM_LOG_ERROR << "A user with this name already exists: " << newName.string();
        return StatusCode::ALREADY_EXISTS;
    }

    UserPtr user = *it;
    user->updateName(std::move(newName));

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
        FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->info() = User::InfoType(newInfo);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::changeUserTitle(IdTypeRef id, StringView newTitle, OutStream& output)
{
    StatusWriter status(output);

    auto config = getGlobalConfig();
    auto validationCode = validateString(newTitle, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->user.minTitleLength, config->user.maxTitleLength);

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

                           if ( ! (status = authorization_->changeUserTitle(*currentUser, user, newTitle)))
                           {
                               return;
                           }

                           if ( ! (status = changeUserTitle(collection, id, newTitle))) return;

                           writeEvents().onChangeUser(createObserverContext(*currentUser), user, User::ChangeType::Title);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserTitle(EntityCollection& collection, IdTypeRef id, StringView newTitle)
{
    auto& indexById = collection.users().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->title() = User::TitleType(newTitle);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::changeUserSignature(IdTypeRef id, StringView newSignature, OutStream& output)
{
    StatusWriter status(output);

    auto config = getGlobalConfig();
    auto validationCode = validateString(newSignature, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->user.minSignatureLength, config->user.maxSignatureLength);

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

                           if ( ! (status = authorization_->changeUserSignature(*currentUser, user, newSignature)))
                           {
                               return;
                           }

                           if ( ! (status = changeUserSignature(collection, id, newSignature))) return;

                           writeEvents().onChangeUser(createObserverContext(*currentUser), user, User::ChangeType::Signature);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserSignature(EntityCollection& collection, IdTypeRef id, StringView newSignature)
{
    auto& indexById = collection.users().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->signature() = User::SignatureType(newSignature);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::changeUserLogo(IdTypeRef id, StringView newLogo, OutStream& output)
{
    StatusWriter status(output);

    auto config = getGlobalConfig();
    auto validationCode = validateImage(newLogo, config->user.maxLogoBinarySize, config->user.maxLogoWidth,
                                        config->user.maxLogoHeight);

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

                           if ( ! (status = authorization_->changeUserLogo(*currentUser, user, newLogo)))
                           {
                               return;
                           }

                           if ( ! (status = changeUserLogo(collection, id, newLogo))) return;

                           writeEvents().onChangeUser(createObserverContext(*currentUser), user, User::ChangeType::Logo);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserLogo(EntityCollection& collection, IdTypeRef id, StringView newLogo)
{
    auto& indexById = collection.users().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->logo() = toString(newLogo);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::deleteUserLogo(IdTypeRef id, OutStream& output)
{
    StatusWriter status(output);

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

                           if ( ! (status = authorization_->deleteUserLogo(*currentUser, user)))
                           {
                               return;
                           }

                           if ( ! (status = deleteUserLogo(collection, id))) return;

                           writeEvents().onChangeUser(createObserverContext(*currentUser), user, User::ChangeType::Logo);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::deleteUserLogo(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.users().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->logo() = {};

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
        FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    collection.deleteUser(*it);

    return StatusCode::OK;
}
