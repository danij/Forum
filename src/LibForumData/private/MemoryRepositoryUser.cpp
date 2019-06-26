/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include <boost/thread/tss.hpp>

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

static constexpr size_t MaxNrOfUserNameChars16 = 65536;
static constexpr size_t MaxNrOfUserNameChars32 = MaxNrOfUserNameChars16 / 2;

static bool isValidUserName(StringView input)
{
    static boost::thread_specific_ptr<UChar> validUserNameBuffer16Ptr;
    static boost::thread_specific_ptr<UChar32> validUserNameBuffer32Ptr;

    if ( ! validUserNameBuffer16Ptr.get())
    {
        validUserNameBuffer16Ptr.reset(new UChar[MaxNrOfUserNameChars16]);
    }
    auto* validUserNameBuffer16 = validUserNameBuffer16Ptr.get();
    
    if ( ! validUserNameBuffer32Ptr.get())
    {
        validUserNameBuffer32Ptr.reset(new UChar32[MaxNrOfUserNameChars32]);
    }
    auto* validUserNameBuffer32 = validUserNameBuffer32Ptr.get();

    //"^[[:alnum:]]+[ _-]*[[:alnum:]]+$"
    if (input.empty())
    {
        return false;
    }

    int32_t written;
    UErrorCode errorCode{};

    const auto u16Chars = u_strFromUTF8Lenient(validUserNameBuffer16, MaxNrOfUserNameChars16, &written,
                                               input.data(), static_cast<int32_t>(input.size()), &errorCode);
    if (U_FAILURE(errorCode)) return false;

    errorCode = {};
    const auto u32Chars = u_strToUTF32(validUserNameBuffer32, MaxNrOfUserNameChars32, &written,
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
      authorizationRepository_(std::move(authorizationRepository))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MemoryRepositoryUser::getCurrentUser(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          status = AuthorizationStatus::OK;
                          status.disable();
                      
                          Json::JsonWriter writer(output);
                      
                          writer.startObject();
                      
                          JSON_WRITE_FIRST_PROP(writer, "authenticated", (! Context::getCurrentUserAuth().empty()));
                      
                          if ( ! isAnonymousUser(currentUser))
                          {
                              const SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                                         currentUser.id(), Context::getCurrentTime());
                              writer.newPropertyRaw(JSON_RAW_PROP_COMMA("user"));
                              serialize(writer, currentUser, restriction);

                              JSON_WRITE_PROP(writer, "newReceivedVotesNr", currentUser.voteHistoryNotRead());
                              JSON_WRITE_PROP(writer, "newReceivedQuotesNr", currentUser.quotesHistoryNotRead());
                              writer.newPropertyRaw(JSON_RAW_PROP_COMMA("newReceivedPrivateMessagesNr")) 
                                      << currentUser.privateMessagesNotRead();
                          }
                      
                          writer.endObject();
                      
                          readEvents().onGetCurrentUser(createObserverContext(currentUser));
                      });
    return status;
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

        SerializationRestriction restriction(collection.grantedPrivileges(), collection, 
                                             currentUser.id(), Context::getCurrentTime());

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
            //collection is sorted in greater order
            writeEntitiesWithPagination(collection.users().byThreadCount(), "users", output, displayContext.pageNumber,
                pageSize, ! ascending, restriction);
            break;
        case RetrieveUsersBy::MessageCount:
            //collection is sorted in greater order
            writeEntitiesWithPagination(collection.users().byMessageCount(), "users", output, displayContext.pageNumber,
                pageSize, ! ascending, restriction);
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

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());

                          auto onlineUsersIntervalSeconds = getGlobalConfig()->user.onlineUsersIntervalSeconds;
                          auto onlineUsersTimeLimit =
                                  static_cast<Timestamp>(Context::getCurrentTime() - onlineUsersIntervalSeconds);

                          Json::JsonWriter writer(output);

                          writer.startObject();
                          writer.newPropertyRaw(JSON_RAW_PROP("online_users"));

                          writer.startArray();

                          auto index = collection.users().byLastSeen();
                          for (auto it = index.rbegin(), itEnd = index.rend(); it != itEnd; ++it)
                          {
                              const User* userPtr = *it;
                              assert(userPtr);

                              const User& user = *userPtr;

                              if ( ! user.showInOnlineUsers()) continue;

                              if (user.lastSeen() < onlineUsersTimeLimit)
                              {
                                  break;
                              }
                              else
                              {
                                  serialize(writer, user, restriction);
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

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());

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

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());

                          writeSingleValueSafeName(output, "user", user, restriction);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getMultipleUsersById(StringView ids, OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    constexpr size_t MaxIdBuffer = 64;
    static boost::thread_specific_ptr<std::array<UuidString, MaxIdBuffer>> parsedIdsPtr;
    static boost::thread_specific_ptr<std::array<const User*, MaxIdBuffer>> usersFoundPtr;

    if ( ! parsedIdsPtr.get())
    {
        parsedIdsPtr.reset(new std::array<UuidString, MaxIdBuffer>);
    }
    auto& parsedIds = *parsedIdsPtr;
    if ( ! usersFoundPtr.get())
    {
        usersFoundPtr.reset(new std::array<const User*, MaxIdBuffer>);
    }
    auto& usersFound = *usersFoundPtr;

    const auto maxUsersToSearch = std::min(MaxIdBuffer, 
                                           static_cast<size_t>(getGlobalConfig()->user.maxUsersPerPage));
    auto lastParsedId = parseMultipleUuidStrings(ids, parsedIds.begin(), parsedIds.begin() + maxUsersToSearch);

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& users = collection.users();
                          const auto index = users.byId();

                          const auto lastUserFound = std::transform(parsedIds.begin(), lastParsedId, usersFound.begin(), 
                              [&index](auto id)
                              {
                                  auto it = index.find(id);
                                  return it == index.end() 
                                      ? nullptr
                                      : *it;
                              });
                          
                          status = StatusCode::OK;
                          status.disable();
                          
                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());

                          writeAllEntities(usersFound.begin(), lastUserFound, "users", output, restriction);
                          
                          readEvents().onGetMultipleUsersById(createObserverContext(currentUser), ids);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getMultipleUsersByName(StringView names, OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    constexpr size_t MaxNameBuffer = 64;
    static boost::thread_specific_ptr<std::array<StringView, MaxNameBuffer>> separatedNamesPtr;
    static boost::thread_specific_ptr<std::array<const User*, MaxNameBuffer>> usersFoundPtr;

    if ( ! separatedNamesPtr.get())
    {
        separatedNamesPtr.reset(new std::array<StringView, MaxNameBuffer>);
    }
    auto& separatedNames = *separatedNamesPtr;
    if ( ! usersFoundPtr.get())
    {
        usersFoundPtr.reset(new std::array<const User*, MaxNameBuffer>);
    }
    auto& usersFound = *usersFoundPtr;

    const auto maxUsersToSearch = std::min(MaxNameBuffer, 
                                           static_cast<size_t>(getGlobalConfig()->user.maxUsersPerPage));
    auto lastName = splitString(names, ',', separatedNames.begin(), separatedNames.begin() + maxUsersToSearch);

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& users = collection.users();
                          const auto index = users.byName();

                          const auto lastUserFound = std::transform(separatedNames.begin(), lastName, usersFound.begin(), 
                              [&index](auto name)
                              {
                                  const auto it = index.find(User::NameType{ name });
                                  return it == index.end() 
                                      ? nullptr
                                      : *it;
                              });
                          
                          status = StatusCode::OK;
                          status.disable();
                          
                          Json::JsonWriter writer(output);
                          writer.startObject();
                          writer.newPropertyRaw(JSON_RAW_PROP("user_ids"));
                          writer.startArray();

                          for (auto it = usersFound.begin(); it != lastUserFound; ++it)
                          {
                              const User* user = *it;
                              if (nullptr == user)
                              {
                                  writer.null();
                              }
                              else
                              {
                                  writer << user->id();
                              }
                          }
        
                          writer.endArray();
                          writer.endObject();
                                                    
                          readEvents().onGetMultipleUsersByName(createObserverContext(currentUser), names);
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

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());

                          status.writeNow([&](auto& writer)
                                          {
                                              JSON_WRITE_PROP(writer, "index", boundIndex);
                                              JSON_WRITE_PROP(writer, "pageSize", getGlobalConfig()->user.maxUsersPerPage);
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

                          const auto& logo = user.logo().string();
                          output.write(logo.data(), logo.size());

                          readEvents().onGetUserLogo(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getUserVoteHistory(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);
                                                    
                          if (isAnonymousUser(currentUser))
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          status = StatusCode::OK;
                          status.disable();

                          Json::JsonWriter writer(output);
                          writer.startObject();
                          writer.newPropertyRaw(JSON_RAW_PROP("lastRetrievedAt"))
                                  << currentUser.voteHistoryLastRetrieved().exchange(Context::getCurrentTime());

                          writer.newPropertyRaw(JSON_RAW_PROP_COMMA("receivedVotes"));
                          writer.startArray();

                          const auto& messageIndex = collection.threadMessages().byId();
                          const SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                                     currentUser.id(), Context::getCurrentTime());
                          BoolTemporaryChanger _(serializationSettings.hideLatestMessage, true);
                          BoolTemporaryChanger __(serializationSettings.hidePrivileges, true);
                          BoolTemporaryChanger ___(serializationSettings.hideDiscussionThreadCreatedBy, true);
                          BoolTemporaryChanger ____(serializationSettings.hideDiscussionThreadMessageCreatedBy, true);

                          const auto& voteHistory = currentUser.voteHistory();

                          for (auto it = voteHistory.rbegin(); it != voteHistory.rend(); ++it)
                          {
                              const User::ReceivedVoteHistory& entry = *it;

                              writer.startObject();

                              JSON_WRITE_FIRST_PROP(writer, "at", entry.at);

                              writer.newPropertyRaw(JSON_RAW_PROP_COMMA("score"));

                              switch (entry.type)
                              {
                              case User::ReceivedVoteHistoryEntryType::UpVote:
                                  writer << 1;
                                  break;
                              case User::ReceivedVoteHistoryEntryType::DownVote:
                                  writer << -1;
                                  break;
                              case User::ReceivedVoteHistoryEntryType::ResetVote:
                                  writer << 0;
                                  break;
                              }

                              writer.newPropertyRaw(JSON_RAW_PROP_COMMA("message"));

                              const auto messageIt = messageIndex.find(entry.discussionThreadMessageId);
                              if (messageIt != messageIndex.end())
                              {
                                  auto& message = **messageIt;
                                  serialize(writer, message, restriction);
                              }
                              else
                              {
                                  writer.null();
                              }

                              writer.endObject();
                          }

                          writer.endArray();
                          writer.endObject();

                          readEvents().onGetUserVoteHistory(createObserverContext(currentUser));

                          currentUser.voteHistoryNotRead() = 0;
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getUserQuotedHistory(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);
                                                    
                          if (isAnonymousUser(currentUser))
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          status = StatusCode::OK;
                          status.disable();

                          Json::JsonWriter writer(output);
                          writer.startObject();

                          writer.newPropertyRaw(JSON_RAW_PROP("messages"));
                          writer.startArray();

                          const auto& messageIndex = collection.threadMessages().byId();
                          const SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                                     currentUser.id(), Context::getCurrentTime());
                          BoolTemporaryChanger _(serializationSettings.hideLatestMessage, true);
                          BoolTemporaryChanger __(serializationSettings.hidePrivileges, true);
                          BoolTemporaryChanger ___(serializationSettings.hideDiscussionThreadCreatedBy, true);

                          const auto& quoteHistory = currentUser.quoteHistory();

                          for (auto it = quoteHistory.rbegin(); it != quoteHistory.rend(); ++it)
                          {
                              const auto messageIt = messageIndex.find(*it);
                              if (messageIt != messageIndex.end())
                              {
                                  auto& message = **messageIt;
                                  serialize(writer, message, restriction);
                              }
                              else
                              {
                                  writer.null();
                              }
                          }

                          writer.endArray();
                          writer.endObject();

                          readEvents().onGetUserQuotedHistory(createObserverContext(currentUser));

                          currentUser.quotesHistoryNotRead() = 0;
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getReceivedPrivateMessages(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);
                                                    
                          if (isAnonymousUser(currentUser))
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          
                          status = StatusCode::OK;
                          status.disable();
                          
                          const auto pageSize = getGlobalConfig()->privateMessage.maxMessagesPerPage;
                          const auto& displayContext = Context::getDisplayContext();

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());
                          BoolTemporaryChanger _(serializationSettings.hidePrivateMessageDestination, true);
                          BoolTemporaryChanger __(serializationSettings.allowDisplayPrivateMessageIpAddress, 
                                  restriction.isAllowed(ForumWidePrivilege::VIEW_PRIVATE_MESSAGE_IP_ADDRESS));

                          writeEntitiesWithPagination(currentUser.receivedPrivateMessages().byCreated(), "messages",
                                                      output, displayContext.pageNumber, pageSize, false, restriction);
                          
                          readEvents().onGetUserReceivedPrivateMessages(createObserverContext(currentUser));

                          currentUser.privateMessagesNotRead() = 0;
                      });
    return status;
}

StatusCode MemoryRepositoryUser::getSentPrivateMessages(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);
                                                    
                          if (isAnonymousUser(currentUser))
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          
                          status = StatusCode::OK;
                          status.disable();
                          
                          const auto pageSize = getGlobalConfig()->privateMessage.maxMessagesPerPage;
                          const auto& displayContext = Context::getDisplayContext();

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());
                          BoolTemporaryChanger _(serializationSettings.hidePrivateMessageSource, true);
                          BoolTemporaryChanger __(serializationSettings.allowDisplayPrivateMessageIpAddress,
                                  restriction.isAllowed(ForumWidePrivilege::VIEW_PRIVATE_MESSAGE_IP_ADDRESS));

                          writeEntitiesWithPagination(currentUser.sentPrivateMessages().byCreated(), "messages",
                                                      output, displayContext.pageNumber, pageSize, false, restriction);

                          readEvents().onGetUserSentPrivateMessages(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryUser::addNewUser(StringView name, StringView auth, OutStream& output)
{
    StatusWriter status(output);

    if (auth.empty())
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING,
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

                           if ( ! isAnonymousUser(currentUser))
                           {
                               status = AuthorizationStatus::NOT_ALLOWED;
                               return;
                           }

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
                               authorizationRepository_->assignForumWidePrivilege(collection, user->id(), 
                                       MaxPrivilegeValue, UnlimitedDuration);
                               writeEvents().onAssignForumWidePrivilege(createObserverContext(*currentUser),
                                   *user, MaxPrivilegeValue, UnlimitedDuration);
                           }

                           status.writeNow([&](auto& writer)
                                           {
                                               JSON_WRITE_PROP(writer, "id", user->id());
                                               JSON_WRITE_PROP(writer, "name", user->name().string());
                                               JSON_WRITE_PROP(writer, "created", user->created());
                                           });
                       });
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
    const auto config = getGlobalConfig();
    const auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
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
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << id.toStringDashed();
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

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(newInfo, ALLOW_EMPTY_STRING,
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
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->info() = User::InfoType(newInfo);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::changeUserTitle(IdTypeRef id, StringView newTitle, OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(newTitle, ALLOW_EMPTY_STRING,
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
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->title() = User::TitleType(newTitle);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::changeUserSignature(IdTypeRef id, StringView newSignature, OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(newSignature, ALLOW_EMPTY_STRING,
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
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->signature() = User::SignatureType(newSignature);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::changeUserAttachmentQuota(IdTypeRef id, const uint64_t newQuota, OutStream& output)
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

                           if ( ! (status = authorization_->changeUserAttachmentQuota(*currentUser, user, newQuota)))
                           {
                               return;
                           }

                           if ( ! (status = changeUserAttachmentQuota(collection, id, newQuota))) return;

                           writeEvents().onChangeUser(createObserverContext(*currentUser), user, 
                                                      User::ChangeType::AttachmentQuota);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::changeUserAttachmentQuota(EntityCollection& collection, IdTypeRef id, 
                                                           const uint64_t newQuota)
{
    auto& indexById = collection.users().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->attachmentQuota() = newQuota;
    
    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::changeUserLogo(IdTypeRef id, StringView newLogo, OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();
    const auto validationCode = validateImage(newLogo, config->user.maxLogoBinarySize, config->user.maxLogoWidth,
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
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->logo() = User::LogoType(newLogo);

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
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    UserPtr user = *it;
    user->logo() = User::LogoType({});

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
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    collection.deleteUser(*it);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryUser::sendPrivateMessage(IdTypeRef destinationId, StringView content, OutStream& output)
{
    StatusWriter status(output);
    if ( ! destinationId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(content, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->privateMessage.minContentLength,
                                               config->privateMessage.maxContentLength,
                                               &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& usersIndex = collection.users().byId();
                           auto usersIt = usersIndex.find(destinationId);
                           if (usersIt == usersIndex.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->sendPrivateMessage(*currentUser, **usersIt, content)))
                           {
                               return;
                           }

                           auto statusWithResource = sendPrivateMessage(collection, generateUniqueId(),
                                                                        destinationId, content);
                           if ( ! (status = statusWithResource.status)) return;

                           auto& messagePtr = statusWithResource.resource;
                           writeEvents().onSendPrivateMessage(createObserverContext(*currentUser), *messagePtr);
                       });
    return status;
}

StatusWithResource<PrivateMessagePtr> MemoryRepositoryUser::sendPrivateMessage(EntityCollection& collection, 
                                                                               IdTypeRef messageId,
                                                                               IdTypeRef destinationId, 
                                                                               const StringView content)
{
    User& sourceUser = *Repository::getCurrentUser(collection);

    auto& messageIndexById = collection.privateMessages().byId();
    if (messageIndexById.find(messageId) != messageIndexById.end())
    {
        FORUM_LOG_ERROR << "A private message with this id already exists: " << messageId.toStringDashed();
        return StatusCode::ALREADY_EXISTS;
    }

    auto& userIndexById = collection.users().byId();
    const auto it = userIndexById.find(destinationId);
    if (it == userIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find user: " << destinationId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }
    UserPtr destinationUserPtr = *it;
    User& destinationUser = *destinationUserPtr;

    const auto messagePtr = collection.createPrivateMessage(messageId, sourceUser, destinationUser, 
                                                            Context::getCurrentTime(),
                                                            { Context::getCurrentUserIpAddress() }, 
                                                            PrivateMessage::ContentType(content));
    collection.insertPrivateMessage(messagePtr);

    sourceUser.sentPrivateMessages().add(messagePtr);
    destinationUser.receivedPrivateMessages().add(messagePtr);
    destinationUser.privateMessagesNotRead() += 1;

    return messagePtr;
}

StatusCode MemoryRepositoryUser::deletePrivateMessage(IdTypeRef id, OutStream& output)
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
                           auto& indexById = collection.privateMessages().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           if ( ! (status = authorization_->deletePrivateMessage(*currentUser, **it)))
                           {
                               return;
                           }

                           //make sure the message is not deleted before being passed to the observers
                           writeEvents().onDeletePrivateMessage(createObserverContext(*currentUser), **it);

                           status = deletePrivateMessage(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryUser::deletePrivateMessage(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.privateMessages().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find private message: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    collection.deletePrivateMessage(*it);

    return StatusCode::OK;
}

void MemoryRepositoryUser::updateCurrentUserId()
{
    auto currentUserAuth = Context::getCurrentUserAuth();
    if (currentUserAuth.empty()) return;

    collection().read([&](const EntityCollection& collection)
                      {
                          const auto& indexByAuth = collection.users().byAuth();
                          const auto it = indexByAuth.find(currentUserAuth);
                      
                          if (it == indexByAuth.end()) return;
                      
                          const User& user = **it;
                          Context::setCurrentUserId(user.id());
                      });
}
